/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#include <msquic.hpp>
#include <stdio.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "top_domains.h"

#ifdef _WIN32
#define QUIC_CALL __cdecl
#else
#define QUIC_CALL
#endif

using namespace std;

const MsQuicApi* MsQuic;

struct ReachConfig {
    std::vector<const char*> HostNames;
    uint16_t Port {443};
    MsQuicAlpn Alpn {"h3"};
    MsQuicSettings Settings;
    QUIC_CREDENTIAL_FLAGS CredFlags {QUIC_CREDENTIAL_FLAG_CLIENT};

    ReachConfig() {
        Settings.SetHandshakeIdleTimeoutMs(1000);
        Settings.SetPeerUnidiStreamCount(3);
    }
};

bool ParseConfig(int argc, char **argv, ReachConfig& Config) {
    if (argc < 2 || (!strcmp(argv[1], "?") || !strcmp(argv[1], "help"))) {
        printf("Usage: quicreach <hostname(s)> [options]\n");
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
        } else if (Event->Type == QUIC_CONNECTION_EVENT_CONNECTED) {
            Connection->SetHandshakeComplete();
        } else if (Event->Type == QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED) {
            //
            // Not great beacuse it doesn't provide an application specific
            // error code. If you expect to get streams, you should not no-op
            // the callbacks.
            //
            MsQuic->StreamClose(Event->PEER_STREAM_STARTED.Stream);
        }
        return QUIC_STATUS_SUCCESS;
    }
};

bool TestReachability(const ReachConfig& Config) {
    MsQuicRegistration Registration("quicreach");
    MsQuicConfiguration Configuration(Registration, Config.Alpn, Config.Settings, MsQuicCredentialConfig(Config.CredFlags));
    if (!Configuration.IsValid()) { printf("Configuration initializtion failed!\n"); return false; }

    printf("%26s          TIME           RTT    SEND:RECV           STATS\n", "SERVER");

    uint32_t ReachableCount = 0, UnreachableCount = 0;
    for (auto HostName : Config.HostNames) {
        ReachConnection Connection(Registration);
        if (!Connection.IsValid()) { printf("Connection initializtion failed!\n"); return false; }
        if (QUIC_FAILED(Connection.Start(Configuration, HostName, Config.Port))) {
            printf("Connection start failed!\n"); return false;
        }

        Connection.WaitOnHandshakeComplete();
        if (Connection.HandshakeSuccess) {
            auto Time = (uint32_t)(Connection.Stats.TimingHandshakeFlightEnd - Connection.Stats.TimingStart);
            printf("%26s    %3u.%03u ms    %3u.%03u ms    %llu:%llu (%u.%1ux)    %u RX CRYPTO\n",
                HostName,
                Time / 1000, Time % 1000,
                Connection.Stats.Rtt / 1000, Connection.Stats.Rtt % 1000,
                Connection.Stats.SendTotalBytes,
                Connection.Stats.RecvTotalBytes,
                (uint32_t)(Connection.Stats.RecvTotalBytes / Connection.Stats.SendTotalBytes),
                (uint32_t)((Connection.Stats.RecvTotalBytes % Connection.Stats.SendTotalBytes) / 100),
                Connection.Stats.HandshakeServerFlight1Bytes);
            ++ReachableCount;
        } else {
            printf("%26s\n", HostName);
            ++UnreachableCount;
        }
    }

    if (ReachableCount > 1) printf("\n%u domains reachable\n", ReachableCount);

    return ReachableCount != 0;
}

int QUIC_CALL main(int argc, char **argv) {

    ReachConfig Config;
    if (!ParseConfig(argc, argv, Config)) {
        return 1;
    }

    MsQuic = new (std::nothrow) MsQuicApi();
    if (!MsQuic || QUIC_FAILED(MsQuic->GetInitStatus())) {
        printf("MsQuicApi failed, 0x%x\n", MsQuic->GetInitStatus());
        return 1;
    }

    bool Result = TestReachability(Config);

    delete MsQuic;
    return Result ? 0 : 1;
}
