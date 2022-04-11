/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

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
    ReachConfig() {
        Settings.SetHandshakeIdleTimeoutMs(750);
        Settings.SetPeerUnidiStreamCount(3);
    }
};

bool ParseConfig(int argc, char **argv, ReachConfig& Config) {
    if (argc < 2 || !strcmp(argv[1], "-?") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        printf("usage: quicreach <hostname(s)> [options...]\n"
               " -a, --alpn <alpn>            The ALPN to use for the handshake (def=h3)\n"
               " -b, --built-in-validation    Use built-in TLS validation logic\n"
               " -h, --help                   Prints this help text\n"
               " -p, --port <port>            The default UDP port to use\n"
               " -r, --req-all                Require all hostnames to succeed\n"
               " -s, --stats                  Print connection statistics\n"
               " -u, --unsecure               Allows unsecure connections\n"
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
        } else if (!strcmp(argv[i], "--built-in-validation") || !strcmp(argv[i], "-b")) {
            Config.CredFlags |= QUIC_CREDENTIAL_FLAG_USE_TLS_BUILTIN_CERTIFICATE_VALIDATION;
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
            Connection->SetHandshakeComplete();
        } else if (Event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE) {
            Connection->SetHandshakeComplete();
        } else if (Event->Type == QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED) {
            MsQuic->StreamClose(Event->PEER_STREAM_STARTED.Stream); // Shouldn't do this
        }
        return QUIC_STATUS_SUCCESS;
    }
};

bool TestReachability(const ReachConfig& Config) {
    MsQuicRegistration Registration("quicreach");
    MsQuicConfiguration Configuration(Registration, Config.Alpn, Config.Settings, MsQuicCredentialConfig(Config.CredFlags));
    if (!Configuration.IsValid()) { printf("Configuration initializtion failed!\n"); return false; }

    if (Config.PrintStatistics)
        printf("%30s           RTT        TIME_I        TIME_H           SEND:RECV      C1      S1\n", "SERVER");

    uint32_t ReachableCount = 0;
    for (auto HostName : Config.HostNames) {
        if (Config.PrintStatistics) printf("%30s", HostName);
        ReachConnection Connection(Registration);
        if (!Connection.IsValid()) { printf("Connection initializtion failed!\n"); return false; }
        if (QUIC_FAILED(Connection.Start(Configuration, HostName, Config.Port))) {
            printf("Connection start failed!\n"); return false;
        }

        Connection.WaitOnHandshakeComplete();
        if (Connection.HandshakeSuccess) {
            auto HandshakeTime = (uint32_t)(Connection.Stats.TimingHandshakeFlightEnd - Connection.Stats.TimingStart);
            auto InitialTime = (uint32_t)(Connection.Stats.TimingInitialFlightEnd - Connection.Stats.TimingStart);
            auto Amplification = (double)Connection.Stats.RecvTotalBytes / (double)Connection.Stats.SendTotalBytes;
            if (Config.PrintStatistics)
                printf("    %3u.%03u ms    %3u.%03u ms    %3u.%03u ms    %u:%u (%2.1fx)    %4u    %4u",
                        Connection.Stats.Rtt / 1000, Connection.Stats.Rtt % 1000,
                        InitialTime / 1000, InitialTime % 1000,
                        HandshakeTime / 1000, HandshakeTime % 1000,
                        (uint32_t)Connection.Stats.SendTotalBytes,
                        (uint32_t)Connection.Stats.RecvTotalBytes,
                        Amplification,
                        Connection.Stats.HandshakeClientFlight1Bytes,
                        Connection.Stats.HandshakeServerFlight1Bytes);
            ++ReachableCount;
        }
        if (Config.PrintStatistics) printf("\n");
    }

    if (Config.PrintStatistics && ReachableCount > 1)
        printf("\n%u domains reachable\n", ReachableCount);
    return Config.RequireAll ? ((size_t)ReachableCount == Config.HostNames.size()) : (ReachableCount != 0);
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
