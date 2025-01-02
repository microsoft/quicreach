/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#define _CRT_SECURE_NO_WARNINGS 1
#define QUIC_API_ENABLE_PREVIEW_FEATURES 1
#define QUICREACH_VERSION_ONLY 1

#include <stdio.h>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <msquic.hpp>
#include "quicreach.ver"
#include "domains.hpp"

#ifdef _WIN32
#define QUIC_CALL __cdecl
#else
#define QUIC_CALL
#endif

const MsQuicApi* MsQuic;

// TODO - Make these public?
#define QUIC_VERSION_2          0x6b3343cfU     // Second official version (host byte order)
#define QUIC_VERSION_1          0x00000001U     // First official version (host byte order)

#define LOW_AMPLIFICATION_LIMIT     3.0         // Limit as specified by the QUIC spec
#define HIGH_AMPLIFICATION_LIMIT    5.0         // Higher limit that seems to be practically used

const uint32_t SupportedVersions[] = {QUIC_VERSION_1, QUIC_VERSION_2};
const MsQuicVersionSettings VersionSettings(SupportedVersions, 2);

struct ReachConfig {
    bool PrintStatistics {false};
    bool RequireAll {false};
    std::vector<const char*> HostNames;
    QuicAddr Address;
    QuicAddr SourceAddress;
    uint32_t Parallel {1};
    uint32_t Repeat {0};
    uint32_t Timeout {1000};
    uint16_t Port {443};
    MsQuicAlpn Alpn {"h3"};
    MsQuicSettings Settings;
    QUIC_CREDENTIAL_FLAGS CredFlags {QUIC_CREDENTIAL_FLAG_CLIENT};
    const char* OutCsvFile {nullptr};
    ReachConfig() { }
    void Set() {
        Settings.SetDisconnectTimeoutMs(Timeout);
        Settings.SetHandshakeIdleTimeoutMs(Timeout);
        Settings.SetPeerUnidiStreamCount(3);
        Settings.SetMinimumMtu(1288); /* We use a slightly larger than default MTU:
                                         1240 (QUIC) + 40 (IPv6) + 8 (UDP) */
        Settings.SetMaximumMtu(1500);
    }
} Config;

struct ReachResults {
    std::atomic<uint32_t> TotalCount {0};
    std::atomic<uint32_t> ReachableCount {0};
    std::atomic<uint32_t> TooMuchCount {0};
    std::atomic<uint32_t> WayTooMuchCount {0};
    std::atomic<uint32_t> MultiRttCount {0};
    std::atomic<uint32_t> RetryCount {0};
    std::atomic<uint32_t> IPv6Count {0};
    std::atomic<uint32_t> Quicv2Count {0};
    // Number of currently active connections.
    uint32_t ActiveCount {0};
    // Synchronization for active count.
    std::mutex Mutex;
    std::condition_variable NotifyEvent;
    void WaitForActiveCount() {
        while (ActiveCount >= Config.Parallel) {
            std::unique_lock<std::mutex> lock(Mutex);
            NotifyEvent.wait(lock, [this]() { return ActiveCount < Config.Parallel; });
        }
    }
    void WaitForAll() {
        while (ActiveCount) {
            std::unique_lock<std::mutex> lock(Mutex);
            NotifyEvent.wait(lock, [this]() { return ActiveCount == 0; });
        }
    }
    void IncActive() {
        std::lock_guard<std::mutex> lock(Mutex);
        ++ActiveCount;
    }
    void DecActive() {
        std::unique_lock<std::mutex> lock(Mutex);
        ActiveCount--;
        NotifyEvent.notify_all();
    }
} Results;

void AddHostName(char* arg) {
    // Parse hostname(s), treating '*' as all top-level domains.
    if (!strcmp(arg, "*")) {
        for (const auto& Domain : TopDomains) {
            Config.HostNames.push_back(Domain);
        }
    } else {
        char* HostName = arg;
        do {
            char* End = strchr(HostName, ',');
            if (End) *End = '\0';
            Config.HostNames.push_back(HostName);
            if (!End) break;
            HostName = End + 1;
        } while (true);
    }
}

bool ParseConfig(int argc, char **argv) {
    if (argc < 2 || !strcmp(argv[1], "-?") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        printf("usage: quicreach <hostname(s)> [options...]\n"
               " -a, --alpn <alpn>      The ALPN to use for the handshake (def=h3)\n"
               " -b, --built-in-val     Use built-in TLS validation logic\n"
               " -c, --csv <file>       Writes CSV results to the given file\n"
               " -h, --help             Prints this help text\n"
               " -i, --ip <address>     The IP address to use\n"
               " -l, --parallel <num>   The numer of parallel hosts to test at once (def=1)\n"
               " -m, --mtu <mtu>        The initial (IPv6) MTU to use (def=1288)\n"
               " -p, --port <port>      The UDP port to use (def=443)\n"
               " -r, --req-all          Require all hostnames to succeed\n"
               " -R, --repeat <time>    Repeat the requests event N milliseconds\n"
               " -s, --stats            Print connection statistics\n"
               " -S, --source <address> Specify a source IP address\n"
               " -t, --timeout <time>   Timeout in milliseconds to wait for each handshake\n"
               " -u, --unsecure         Allows unsecure connections\n"
               " -v, --version          Prints out the version\n"
              );
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            AddHostName(argv[i]);

        } else if (!strcmp(argv[i], "--alpn") || !strcmp(argv[i], "-a")) {
            if (++i >= argc) { printf("Missing ALPN string\n"); return false; }
            Config.Alpn = argv[i];

        } else if (!strcmp(argv[i], "--built-in-val") || !strcmp(argv[i], "-b")) {
            Config.CredFlags |= QUIC_CREDENTIAL_FLAG_USE_TLS_BUILTIN_CERTIFICATE_VALIDATION;

        } else if (!strcmp(argv[i], "--csv") || !strcmp(argv[i], "-c")) {
            if (++i >= argc) { printf("Missing file name\n"); return false; }
            Config.OutCsvFile = argv[i];

        } else if (!strcmp(argv[i], "--mtu") || !strcmp(argv[i], "-m")) {
            if (++i >= argc) { printf("Missing MTU value\n"); return false; }
            Config.Settings.SetMinimumMtu((uint16_t)atoi(argv[i]));

        } else if (!strcmp(argv[i], "--ip") || !strcmp(argv[i], "-i")) {
            if (++i >= argc) { printf("Missing IP address\n"); return false; }
            if (!QuicAddrFromString(argv[i], 0, &Config.Address.SockAddr)) {
                printf("Invalid address arg\n"); return false;
            }

        } else if (!strcmp(argv[i], "--parallel") || !strcmp(argv[i], "-l")) {
            if (++i >= argc) { printf("Missing parallel number\n"); return false; }
            Config.Parallel = (uint32_t)atoi(argv[i]);

        } else if (!strcmp(argv[i], "--port") || !strcmp(argv[i], "-p")) {
            if (++i >= argc) { printf("Missing port number\n"); return false; }
            Config.Port = (uint16_t)atoi(argv[i]);

        } else if (!strcmp(argv[i], "--stats") || !strcmp(argv[i], "-s")) {
            Config.PrintStatistics = true;

        } else if (!strcmp(argv[i], "--source") || !strcmp(argv[i], "-S")) {
            if (++i >= argc) { printf("Missing source address\n"); return false; }
            if (!QuicAddrFromString(argv[i], 0, &Config.SourceAddress.SockAddr)) {
                printf("Invalid source address arg\n"); return false;
            }

        } else if (!strcmp(argv[i], "--req-all") || !strcmp(argv[i], "-r")) {
            Config.RequireAll = true;

        } else if (!strcmp(argv[i], "--repeat") || !strcmp(argv[i], "-R")) {
            if (++i >= argc) { printf("Missing repeat arg\n"); return false; }
            Config.Repeat = (uint32_t)atoi(argv[i]);

        } else if (!strcmp(argv[i], "--timeout") || !strcmp(argv[i], "-t")) {
            if (++i >= argc) { printf("Missing timeout arg\n"); return false; }
            Config.Timeout = (uint32_t)atoi(argv[i]);

        } else if (!strcmp(argv[i], "--unsecure") || !strcmp(argv[i], "-u")) {
            Config.CredFlags |= QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;

        } else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
            printf("quicreach " QUICREACH_VERSION "\n");
        }
    }

    Config.Set();

    return true;
}

struct ReachConnection : public MsQuicConnection {
    const char* HostName;
    bool HandshakeComplete {false};
    QUIC_STATISTICS_V2 Stats {0};
    ReachConnection(
        _In_ const MsQuicRegistration& Registration,
        _In_ const MsQuicConfiguration& Configuration,
        _In_ const char* HostName
    ) : MsQuicConnection(Registration, CleanUpAutoDelete, Callback), HostName(HostName) {
        Results.TotalCount++;
        Results.IncActive();
        if (IsValid() && Config.Address.GetFamily() != QUIC_ADDRESS_FAMILY_UNSPEC) {
            InitStatus = SetRemoteAddr(Config.Address);
        }
        if (IsValid() && Config.SourceAddress.GetFamily() != QUIC_ADDRESS_FAMILY_UNSPEC) {
            InitStatus = SetLocalAddr(Config.SourceAddress);
        }
        if (IsValid()) {
            InitStatus = Start(Configuration, HostName, Config.Port);
        }
        if (!IsValid()) {
            Results.DecActive();
        }
    }
    static QUIC_STATUS QUIC_API Callback(
        _In_ MsQuicConnection* _Connection,
        _In_opt_ void* ,
        _Inout_ QUIC_CONNECTION_EVENT* Event
        ) noexcept {
        auto Connection = (ReachConnection*)_Connection;
        if (Event->Type == QUIC_CONNECTION_EVENT_CONNECTED) {
            Connection->OnReachable();
            Connection->Shutdown(0);
        } else if (Event->Type == QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE) {
            if (!Connection->HandshakeComplete) Connection->OnUnreachable();
            Results.DecActive();
        } else if (Event->Type == QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED) {
            MsQuic->StreamClose(Event->PEER_STREAM_STARTED.Stream); // Shouldn't do this
        }
        return QUIC_STATUS_SUCCESS;
    }
private:
    void OnReachable() {
        HandshakeComplete = true;
        Results.ReachableCount++;
        GetStatistics(&Stats);
        QuicAddr RemoteAddr;
        GetRemoteAddr(RemoteAddr);
        uint32_t Version, VersionLength = sizeof(Version);
        GetParam(QUIC_PARAM_CONN_QUIC_VERSION, &VersionLength, &Version);
        auto HandshakeTime = (uint32_t)(Stats.TimingHandshakeFlightEnd - Stats.TimingStart);
        auto InitialTime = (uint32_t)(Stats.TimingInitialFlightEnd - Stats.TimingStart);
        auto Amplification = (double)Stats.RecvTotalBytes / (double)Stats.SendTotalBytes;
        auto TooMuch = false, MultiRtt = false;
        auto Retry = (bool)(Stats.StatelessRetry);
        if (Stats.SendTotalPackets != 1) {
            MultiRtt = true;
            Results.MultiRttCount++;
        } else {
            TooMuch = Amplification > LOW_AMPLIFICATION_LIMIT;
            if (TooMuch) {
                Results.TooMuchCount++;
                if (Amplification > HIGH_AMPLIFICATION_LIMIT) {
                    Results.WayTooMuchCount++;
                }
            }
        }
        if (Retry) {
            Results.RetryCount++;
        }
        if (RemoteAddr.GetFamily() == QUIC_ADDRESS_FAMILY_INET6) {
            Results.IPv6Count++;
        }
        if (Version == QUIC_VERSION_2) {
            Results.Quicv2Count++;
        }
        if (Config.PrintStatistics){
            const char HandshakeTags[3] = {
                TooMuch ? '!' : (MultiRtt ? '*' : ' '),
                Retry ? 'R' : ' ',
                '\0'};
            QUIC_ADDR_STR AddrStr;
            QuicAddrToString(&RemoteAddr.SockAddr, &AddrStr);
            std::unique_lock<std::mutex> lock(Results.Mutex);
            printf("%30s   %3u.%03u ms   %3u.%03u ms   %3u.%03u ms   %u:%u %u:%u (%2.1fx)  %4u   %4u     %s   %20s   %s\n",
                HostName,
                Stats.Rtt / 1000, Stats.Rtt % 1000,
                InitialTime / 1000, InitialTime % 1000,
                HandshakeTime / 1000, HandshakeTime % 1000,
                (uint32_t)Stats.SendTotalPackets,
                (uint32_t)Stats.RecvTotalPackets,
                (uint32_t)Stats.SendTotalBytes,
                (uint32_t)Stats.RecvTotalBytes,
                Amplification,
                Stats.HandshakeClientFlight1Bytes,
                Stats.HandshakeServerFlight1Bytes,
                Version == QUIC_VERSION_1 ? "v1" : "v2",
                AddrStr.Address,
                HandshakeTags);
        }
    }
    void OnUnreachable() {
        if (Config.PrintStatistics) {
            std::unique_lock<std::mutex> lock(Results.Mutex);
            printf("%30s\n", HostName);
        }
    }
};

void DumpResultsToFile() {
    FILE* File = fopen(Config.OutCsvFile, "wx"); // Try to create a new file
    if (!File) {
        File = fopen(Config.OutCsvFile, "a"); // Open an existing file
        if (!File) {
            printf("Failed to open output file: %s\n", Config.OutCsvFile);
            return;
        }
    } else {
        fprintf(File, "UtcDateTime,Total,Reachable,TooMuch,MultiRtt,Retry,IPv6,QuicV2,WayTooMuch\n");
    }
    char UtcDateTime[256];
    time_t Time = time(nullptr);
    struct tm Tm;
#ifdef _WIN32
    gmtime_s(&Tm, &Time);
#else
    gmtime_r(&Time, &Tm);
#endif
    strftime(UtcDateTime, sizeof(UtcDateTime), "%Y.%m.%d-%H:%M:%S", &Tm);
    fprintf(File, "%s,%u,%u,%u,%u,%u,%u,%u,%u\n", UtcDateTime,
        Results.TotalCount.load(), Results.ReachableCount.load(), Results.TooMuchCount.load(), Results.MultiRttCount.load(),
        Results.RetryCount.load(), Results.IPv6Count.load(), Results.Quicv2Count.load(), Results.WayTooMuchCount.load());
    fclose(File);
    printf("\nOutput written to %s\n", Config.OutCsvFile);
}

// TODO:
// - MsQuic should expose HRR flag for handshake?
// - Figure out a way to fingerprint the server implementation?

bool TestReachability() {
    MsQuicRegistration Registration("quicreach");
    MsQuicConfiguration Configuration(Registration, Config.Alpn, Config.Settings, MsQuicCredentialConfig(Config.CredFlags));
    if (!Configuration.IsValid()) { printf("Configuration initializtion failed!\n"); return false; }
    Configuration.SetVersionSettings(VersionSettings);
    Configuration.SetVersionNegotiationExtEnabled();

    if (Config.PrintStatistics)
        printf("%30s          RTT       TIME_I       TIME_H              SEND:RECV    C1     S1    VER                     IP\n", "SERVER");

    do {
        for (auto HostName : Config.HostNames) {
            new ReachConnection(Registration, Configuration, HostName);
            Results.WaitForActiveCount();
        }

        Results.WaitForAll();

        if (Config.Repeat) {
#ifdef _WIN32
            Sleep(Config.Repeat);
#else
            usleep(Config.Repeat * 1000);
#endif
        }

    } while (Config.Repeat);

    if (Config.PrintStatistics) {
        if (Results.ReachableCount > 1) {
            printf("\n");
            printf("%4u domain(s) attempted\n", (uint32_t)Config.HostNames.size());
            printf("%4u domain(s) reachable\n", Results.ReachableCount.load());
            if (Results.MultiRttCount)
                printf("%4u domain(s) required multiple round trips (*)\n", Results.MultiRttCount.load());
            if (Results.TooMuchCount)
                printf("%4u domain(s) exceeded amplification limits (!)\n", Results.TooMuchCount.load());
            if (Results.WayTooMuchCount)
                printf("%4u domain(s) well exceeded amplification limits (5x)\n", Results.WayTooMuchCount.load());
            if (Results.RetryCount)
                printf("%4u domain(s) sent RETRY packets (R)\n", Results.RetryCount.load());
            if (Results.IPv6Count)
                printf("%4u domain(s) used IPv6\n", Results.IPv6Count.load());
            if (Results.Quicv2Count)
                printf("%4u domain(s) used QUIC v2\n", Results.Quicv2Count.load());
        }
    }

    if (Config.OutCsvFile) DumpResultsToFile();

    return Config.RequireAll ? ((size_t)Results.ReachableCount == Config.HostNames.size()) : (Results.ReachableCount != 0);
}

int QUIC_CALL main(int argc, char **argv) {

    if (!ParseConfig(argc, argv) || Config.HostNames.empty()) return 1;

    MsQuic = new (std::nothrow) MsQuicApi();
    if (QUIC_FAILED(MsQuic->GetInitStatus())) {
        printf("MsQuicApi failed, 0x%x\n", MsQuic->GetInitStatus());
        return 1;
    }

    bool Result = TestReachability();
    if (!Config.PrintStatistics) {
        printf("%s\n", Result ? "Success" : "Failure");
    }
    delete MsQuic;
    return Result ? 0 : 1;
}
