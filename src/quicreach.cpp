/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#include <msquic.hpp>
#include <stdio.h>

#ifdef _WIN32
#define QUIC_CALL __cdecl
#else
#define QUIC_CALL
#endif

const MsQuicApi* MsQuic;

int QUIC_CALL main(int argc, char **argv) {
    if (argc < 2 || (!strcmp(argv[1], "?") || !strcmp(argv[1], "help"))) {
        printf("Usage: quicreach <server> [options]\n");
        return 1;
    }

    return 0;
}
