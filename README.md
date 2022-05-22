# quicreach

[![Dashboard](https://img.shields.io/static/v1?label=Reachability&message=Dashboard&color=blue)](https://microsoft.github.io/quicreach/)
[![Build](https://github.com/microsoft/quicreach/actions/workflows/build.yml/badge.svg)](https://github.com/microsoft/quicreach/actions/workflows/build.yml)
[![Reach](https://github.com/microsoft/quicreach/actions/workflows/reach.yml/badge.svg)](https://github.com/microsoft/quicreach/actions/workflows/reach.yml)

This project has two primary purposes:

1. It provides a complete (C++) client sample application built on top of [MsQuic](https://github.com/microsoft/msquic).
2. It is a tool to test the QUIC reachability of a server ([latest raw data](https://github.com/microsoft/quicreach/blob/data/data.csv)).

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
> quicreach google.com
Success
```
```
> quicreach example.com
Failure
```

```Bash
> quicreach '*' --stats
                        SERVER           RTT        TIME_I        TIME_H               SEND:RECV      C1      S1    FAMILY
               quic.aiortc.org    102.082 ms    106.934 ms    240.379 ms    4:5 2523:4900 (1.9x)     274    4547    IPv6     *
              ietf.akaquic.com     98.277 ms    100.906 ms    201.243 ms    3:5 2480:5869 (2.4x)     275    4564    IPv6     *
                 quic.ogre.com
                    quic.rocks
                       mew.org    190.587 ms    190.857 ms    379.342 ms    4:6 2522:6650 (2.6x)     266    4541    IPv6     *
  http3-test.litespeedtech.com
                    msquic.net     76.161 ms     76.582 ms     78.957 ms    1:4 1260:3660 (2.9x)     269    3461    IPv4
                   nghttp2.org
           cloudflare-quic.com     14.688 ms     19.289 ms     24.105 ms    3:7 2480:5129 (2.1x)     278    2667    IPv6     *
          pandora.cm.in.tum.de
```

### Full Help

```
> quicreach --help
usage: quicreach <hostname(s)> [options...]
 -a, --alpn <alpn>      The ALPN to use for the handshake (def=h3)
 -b, --built-in-val     Use built-in TLS validation logic
 -f, --file <file>      Writes the results to the given file
 -h, --help             Prints this help text
 -m, --mtu <mtu>        The initial (IPv6) MTU to use (def=1288)
 -p, --port <port>      The default UDP port to use
 -r, --req-all          Require all hostnames to succeed
 -s, --stats            Print connection statistics
 -u, --unsecure         Allows unsecure connections
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
