/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#include <msquic.hpp>
#include <stdio.h>
#include <thread>
#include <vector>

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

bool TestReachability(const ReachConfig& Config) {
    MsQuicRegistration Registration("quicreach");
    MsQuicConfiguration Configuration(Registration, Config.Alpn, Config.Settings, MsQuicCredentialConfig(Config.CredFlags));
    if (!Configuration.IsValid()) { printf("Configuration initializtion failed!\n"); return false; }

    uint32_t ReachableCount = 0, UnreachableCount = 0;
    for (auto HostName : Config.HostNames) {
        MsQuicConnection Connection(Registration);
        if (!Connection.IsValid()) { printf("Connection initializtion failed!\n"); return false; }
        if (QUIC_FAILED(Connection.Start(Configuration, HostName, Config.Port))) {
            printf("Connection start failed!\n"); return false;
        }

        uint32_t tries = 0;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            ++tries;
        } while (!Connection.HandshakeComplete && tries < 50);

        if (Connection.HandshakeComplete) {
            QUIC_STATISTICS_V2 Stats;
            Connection.GetStatistics(&Stats);
            printf("%26s\treachable\t%3u.%03u ms RTT\t  %u bytes\n",
                HostName,
                Stats.Rtt / 1000, Stats.Rtt % 1000,
                Stats.HandshakeServerFlight1Bytes);
            ++ReachableCount;
        } else {
            printf("%26s\n", HostName);
            ++UnreachableCount;
        }
    }

    if (ReachableCount > 1) printf("\n%u domains reachable\n", ReachableCount);

    return UnreachableCount == 0;
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
