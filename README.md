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
 -a, --alpn <alpn>      The ALPN to use for the handshake (def=h3)
 -b, --built-in-val     Use built-in TLS validation logic
 -h, --help             Prints this help text
 -p, --port <port>      The default UDP port to use
 -r, --req-all          Require all hostnames to succeed
 -s, --stats            Print connection statistics
 -u, --unsecure         Allows unsecure connections
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
                        SERVER           RTT        TIME_I        TIME_H           SEND:RECV      C1      S1
               quic.aiortc.org     68.545 ms     73.855 ms    143.703 ms    2440:4898 (2.0x)     274    4545
              ietf.akaquic.com     89.399 ms     92.660 ms    182.458 ms    2440:5850 (2.4x)     275    4565
                 quic.ogre.com
                    quic.rocks
                       mew.org    177.611 ms    177.872 ms    352.459 ms    2440:6750 (2.8x)     266    4541
  http3-test.litespeedtech.com
                    msquic.net     67.373 ms     67.735 ms    130.850 ms    2440:3729 (1.5x)     269    3461
                   nghttp2.org    158.051 ms    158.364 ms    314.771 ms    2440:4542 (1.9x)     270    4173
           cloudflare-quic.com      1.958 ms      5.841 ms      6.548 ms    1220:5129 (4.2x)     278    2667
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
