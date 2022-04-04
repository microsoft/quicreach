# quicreach

[![Build](https://github.com/microsoft/quicreach/actions/workflows/build.yml/badge.svg)](https://github.com/microsoft/quicreach/actions/workflows/build.yml)
[![Reach](https://github.com/microsoft/quicreach/actions/workflows/reach.yml/badge.svg)](https://github.com/microsoft/quicreach/actions/workflows/reach.yml)

This project has two primary purposes:

1. It provides a complete (C++) client sample application built on top of [MsQuic](https://github.com/microsoft/msquic).
2. It is a tool to test the QUIC reachability of a server.

# Build

```Bash
git clone --recursive https://github.com/microsoft/quicreach
cd quicreach && mkdir build && cd build
```

### Linux
```Bash
cmake -G 'Unix Makefiles' ..
cmake --build .
```

### Windows
```Bash
cmake -G 'Visual Studio 17 2022' -A x64 ..
cmake --build .
```

# Usage

```
> quicreach --help
usage: quicreach <hostname(s)> [options...]
 -a, --alpn         The ALPN to use for the handshake (def=h3)
 -h, --help         Prints this help text
 -p, --port <port>  The default UDP port to use
 -s, --stats        Print connection statistics
 -u, --unsecure     Allows unsecure connections
```

```
> quicreach google.com
Success
```
```
> quicreach example.com
Failure
```

```Bash
> quicreach '*' --stats
                        SERVER          TIME           RTT    SEND:RECV           STATS
               quic.aiortc.org    209.917 ms     99.328 ms    1242:4880 (3.9x)    4545 RX CRYPTO
              ietf.akaquic.com
                 quic.ogre.com
                    quic.rocks
                       mew.org    428.800 ms    212.446 ms    1284:6650 (5.2x)    4541 RX CRYPTO
  http3-test.litespeedtech.com
                    msquic.net    147.155 ms     75.131 ms    1263:3729 (3.0x)    3461 RX CRYPTO
                   nghttp2.org
           cloudflare-quic.com     27.666 ms     14.006 ms    1200:5130 (4.3x)    2668 RX CRYPTO
          pandora.cm.in.tum.de
```

# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft
trademarks or logos is subject to and must follow
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
