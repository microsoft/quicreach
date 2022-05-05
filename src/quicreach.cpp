/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <msquic.hpp>
#include "domains.hpp"

#ifdef _WIN32
#define QUIC_CALL __cdecl
#else
#define QUIC_CALL
#endif

using namespace std;

const MsQuicApi* MsQuic;

struct ReachConfig {
    bool PrintStatistics {false};
    bool RequireAll {false};
    std::vector<const char*> HostNames;
    uint16_t Port {443};
    MsQuicAlpn Alpn {"h3"};
    MsQuicSettings Settings;
    QUIC_CREDENTIAL_FLAGS CredFlags {QUIC_CREDENTIAL_FLAG_CLIENT};
    const char* OutputFile {nullptr};
    ReachConfig() {
        Settings.SetDisconnectTimeoutMs(500);
        Settings.SetHandshakeIdleTimeoutMs(750);
        Settings.SetPeerUnidiStreamCount(3);
        Settings.SetMinimumMtu(1288); /* We use a slightly larger than default MTU:
                                         1240 (QUIC) + 40 (IPv6) + 8 (UDP) */
        Settings.SetMaximumMtu(1500);
    }
};

struct ReachResults {
    uint32_t TotalCount {0};
    uint32_t ReachableCount {0};
    uint32_t TooMuchCount {0};
    uint32_t MultiRttCount {0};
    uint32_t RetryCount {0};
};

bool ParseConfig(int argc, char **argv, ReachConfig& Config) {
    if (argc < 2 || !strcmp(argv[1], "-?") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        printf("usage: quicreach <hostname(s)> [options...]\n"
               " -a, --alpn <alpn>      The ALPN to use for the handshake (def=h3)\n"
               " -b, --built-in-val     Use built-in TLS validation logic\n"
               " -f, --file <file>      Writes the results to the given file\n"
               " -h, --help             Prints this help text\n"
               " -m, --mtu <mtu>        The initial (IPv6) MTU to use (def=1288)\n"
               " -p, --port <port>      The default UDP port to use\n"
               " -r, --req-all          Require all hostnames to succeed\n"
               " -s, --stats            Print connection statistics\n"
               " -u, --unsecure         Allows unsecure connections\n"
              );
        return false;
    }

    // Parse hostname(s), treating '*' as all top-level domains.
    if (!strcmp(argv[1], "*")) {
        for (auto Domain : TopDomains) {
            Config.HostNames.push_back(Domain);
        }
    } else {
        char* HostName = (char*)argv[1];
        do {
            char* End = strchr(HostName, ',');
            if (End) *End = 0;
            Config.HostNames.push_back(HostName);
            if (!End) break;
            HostName = End + 1;
        } while (true);
    }

    // Parse options.
    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "--alpn") || !strcmp(argv[i], "-a")) {
            if (++i >= argc) { printf("Missing ALPN string\n"); return false; }
            Config.Alpn = argv[i];

        } else if (!strcmp(argv[i], "--built-in-val") || !strcmp(argv[i], "-b")) {
            Config.CredFlags |= QUIC_CREDENTIAL_FLAG_USE_TLS_BUILTIN_CERTIFICATE_VALIDATION;

        } else if (!strcmp(argv[i], "--file") || !strcmp(argv[i], "-f")) {
            if (++i >= argc) { printf("Missing file name\n"); return false; }
            Config.OutputFile = argv[i];

        } else if (!strcmp(argv[i], "--mtu") || !strcmp(argv[i], "-m")) {
            if (++i >= argc) { printf("Missing MTU value\n"); return false; }
            Config.Settings.SetMinimumMtu((uint16_t)atoi(argv[i]));

        } else if (!strcmp(argv[i], "--port") || !strcmp(argv[i], "-p")) {
            if (++i >= argc) { printf("Missing port number\n"); return false; }
            Config.Port = (uint16_t)atoi(argv[i]);

        } else if (!strcmp(argv[i], "--stats") || !strcmp(argv[i], "-s")) {
            Config.PrintStatistics = true;

        } else if (!strcmp(argv[i], "--req-all") || !strcmp(argv[i], "-r")) {
            Config.RequireAll = true;

        } else if (!strcmp(argv[i], "--unsecure") || !strcmp(argv[i], "-u")) {
            Config.CredFlags |= QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;
        }
    }

    return true;
}

struct ReachConnection : public MsQuicConnection {
    mutex HandshakeCompleteMutex;
    condition_variable HandshakeCompleteEvent;
    bool HandshakeSuccess {false};
    QUIC_STATISTICS_V2 Stats {0};
    const char* FamilyString = "UNKN";
    ReachConnection(
        _In_ const MsQuicRegistration& Registration
    ) : MsQuicConnection(Registration, CleanUpManual, Callback) { }
    void WaitOnHandshakeComplete() {
        std::unique_lock Lock{HandshakeCompleteMutex};
        HandshakeCompleteEvent.wait_for(Lock, chrono::seconds(1), [&]{return HandshakeComplete;});
    }
    void SetHandshakeComplete() {
        std::lock_guard Lock{HandshakeCompleteMutex};
        HandshakeComplete = true;
        HandshakeCompleteEvent.notify_all();
    }
    static QUIC_STATUS QUIC_API Callback(
        _In_ MsQuicConnection* _Connection,
        _In_opt_ void* ,
        _Inout_ QUIC_CONNECTION_EVENT* Event
        ) noexcept {
        auto Connection = (ReachConnection*)_Connection;
        if (Event->Type == QUIC_CONNECTION_EVENT_CONNECTED) {
            Connection->HandshakeSuccess = true;
            Connection->GetStatistics(&Connection->Stats);
            QuicAddr RemoteAddr;
            if (QUIC_SUCCEEDED(Connection->GetRemoteAddr(RemoteAddr))) {
                Connection->FamilyString = RemoteAddr.GetFamily() == QUIC_ADDRESS_FAMILY_INET6 ? "IPv6" : "IPv4";
            }
            Connection->SetHandshakeComplete();
        } else if (Event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE) {
            Connection->SetHandshakeComplete();
        } else if (Event->Type == QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED) {
            MsQuic->StreamClose(Event->PEER_STREAM_STARTED.Stream); // Shouldn't do this
        }
        return QUIC_STATUS_SUCCESS;
    }
};

void DumpResultsToFile(const ReachConfig &Config, const ReachResults &Results) {
    FILE* File = fopen(Config.OutputFile, "wx"); // Try to create a new file
    if (!File) {
        File = fopen(Config.OutputFile, "a"); // Open an existing file
        if (!File) {
            printf("Failed to open output file: %s\n", Config.OutputFile);
            return;
        }
    } else {
        fprintf(File, "UtcDateTime,Total,Reachable,TooMuch,MultiRtt,Retry\n");
    }
    char UtcDateTime[256];
    time_t Time = time(nullptr);
    struct tm* Tm = gmtime(&Time);
    strftime(UtcDateTime, sizeof(UtcDateTime), "%Y.%m.%d-%H:%M:%S", Tm);
    fprintf(File, "%s,%u,%u,%u,%u,%u\n", UtcDateTime, Results.TotalCount, Results.ReachableCount, Results.TooMuchCount, Results.MultiRttCount, Results.RetryCount);
    fclose(File);
    printf("\nOutput written to %s\n", Config.OutputFile);
}

// TODO:
// - MsQuic should expose HRR flag for handshake?
// - Figure out a way to fingerprint the server implementation?

bool TestReachability(const ReachConfig& Config) {
    MsQuicRegistration Registration("quicreach");
    MsQuicConfiguration Configuration(Registration, Config.Alpn, Config.Settings, MsQuicCredentialConfig(Config.CredFlags));
    if (!Configuration.IsValid()) { printf("Configuration initializtion failed!\n"); return false; }

    if (Config.PrintStatistics)
        printf("%30s           RTT        TIME_I        TIME_H               SEND:RECV      C1      S1    FAMILY\n", "SERVER");

    ReachResults Results;
    for (auto HostName : Config.HostNames) {
        ++Results.TotalCount;
        if (Config.PrintStatistics) printf("%30s", HostName);
        ReachConnection Connection(Registration);
        if (!Connection.IsValid()) { printf("Connection initializtion failed!\n"); return false; }
        if (QUIC_FAILED(Connection.Start(Configuration, HostName, Config.Port))) {
            printf("Connection start failed!\n"); return false;
        }

        Connection.WaitOnHandshakeComplete();
        if (Connection.HandshakeSuccess) {
            ++Results.ReachableCount;
            auto HandshakeTime = (uint32_t)(Connection.Stats.TimingHandshakeFlightEnd - Connection.Stats.TimingStart);
            auto InitialTime = (uint32_t)(Connection.Stats.TimingInitialFlightEnd - Connection.Stats.TimingStart);
            auto Amplification = (double)Connection.Stats.RecvTotalBytes / (double)Connection.Stats.SendTotalBytes;
            auto TooMuch = false, MultiRtt = false;
            auto Retry = (bool)(Connection.Stats.StatelessRetry);
            if (Connection.Stats.SendTotalPackets != 1) {
                MultiRtt = true;
                ++Results.MultiRttCount;
            } else {
                TooMuch = Amplification > 3.0;
                if (TooMuch) ++Results.TooMuchCount;
            }
            if (Retry) {
              ++Results.RetryCount;
            }
            if (Config.PrintStatistics){
                char HandshakeTags[3] = {
                    TooMuch ? '!' : (MultiRtt ? '*' : ' '),
                    Retry ? 'R' : ' ',
                    '\0'};
                printf("    %3u.%03u ms    %3u.%03u ms    %3u.%03u ms    %u:%u %u:%u (%2.1fx)    %4u    %4u    %s     %s",
                    Connection.Stats.Rtt / 1000, Connection.Stats.Rtt % 1000,
                    InitialTime / 1000, InitialTime % 1000,
                    HandshakeTime / 1000, HandshakeTime % 1000,
                    (uint32_t)Connection.Stats.SendTotalPackets,
                    (uint32_t)Connection.Stats.RecvTotalPackets,
                    (uint32_t)Connection.Stats.SendTotalBytes,
                    (uint32_t)Connection.Stats.RecvTotalBytes,
                    Amplification,
                    Connection.Stats.HandshakeClientFlight1Bytes,
                    Connection.Stats.HandshakeServerFlight1Bytes,
                    Connection.FamilyString,
                    HandshakeTags);
            }
        }
        if (Config.PrintStatistics) printf("\n");
    }

    if (Config.PrintStatistics) {
        if (Results.ReachableCount > 1) {
            printf("\n");
            printf("%4u domain(s) attempted\n", (uint32_t)Config.HostNames.size());
            printf("%4u domain(s) reachable\n", Results.ReachableCount);
            if (Results.MultiRttCount)
                printf("%4u domain(s) required multiple round trips (*)\n", Results.MultiRttCount);
            if (Results.TooMuchCount)
                printf("%4u domain(s) exceeded amplification limits (!)\n", Results.TooMuchCount);
            if (Results.RetryCount)
                printf("%4u domain(s) sent RETRY packets (R)\n", Results.RetryCount);
        }
    }

    if (Config.OutputFile) DumpResultsToFile(Config, Results);

    return Config.RequireAll ? ((size_t)Results.ReachableCount == Config.HostNames.size()) : (Results.ReachableCount != 0);
}

int QUIC_CALL main(int argc, char **argv) {

    ReachConfig Config;
    if (!ParseConfig(argc, argv, Config)) return 1;

    MsQuic = new (std::nothrow) MsQuicApi();
    if (QUIC_FAILED(MsQuic->GetInitStatus())) {
        printf("MsQuicApi failed, 0x%x\n", MsQuic->GetInitStatus());
        return 1;
    }

    bool Result = TestReachability(Config);
    if (!Config.PrintStatistics) {
        printf("%s\n", Result ? "Success" : "Failure");
    }
    delete MsQuic;
    return Result ? 0 : 1;
}
