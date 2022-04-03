/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#include <msquic.hpp>
#include <stdio.h>
#include <thread>

#ifdef _WIN32
#define QUIC_CALL __cdecl
#else
#define QUIC_CALL
#endif

const MsQuicApi* MsQuic;

struct ReachConfig {
    const char* ServerName {nullptr};
    uint16_t Port {443};
    MsQuicAlpn Alpns {"h3", "smb"};
    MsQuicSettings Settings;
    QUIC_CREDENTIAL_FLAGS CredFlags {QUIC_CREDENTIAL_FLAG_CLIENT};

    ReachConfig() {
        Settings.SetPeerUnidiStreamCount(3);
    }
};

bool ParseConfig(int argc, char **argv, ReachConfig& Config) {
    if (argc < 2 || (!strcmp(argv[1], "?") || !strcmp(argv[1], "help"))) {
        printf("Usage: quicreach <server> [options]\n");
        return false;
    }
    Config.ServerName = argv[1];
    return true;
}

bool TestReachability(const ReachConfig& Config) {
    MsQuicRegistration Registration("quicreach");
    MsQuicConfiguration Configuration(Registration, Config.Alpns, Config.Settings, MsQuicCredentialConfig(Config.CredFlags));
    MsQuicConnection Connection(Registration);
    if (!Connection.IsValid()) { printf("Connection initializtion failed!\n"); return false; }

    if (QUIC_FAILED(Connection.Start(Configuration, Config.ServerName, Config.Port))) {
        printf("Connection start failed!\n"); return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    printf("Handshake complete = %s\n", Connection.HandshakeComplete ? "true" : "false");

    return true;
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
