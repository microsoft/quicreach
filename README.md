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

```Bash
> quicreach google.com
Success
```
```Bash
> quicreach example.com
Failure
```

```Bash
> quicreach '*' --stats
                        SERVER          RTT       TIME_I       TIME_H              SEND:RECV    C1     S1    VER                     IP
                    google.com     2.409 ms     2.936 ms     5.461 ms   3:7 2520:8370 (3.3x)   287   6901     v1     172.253.62.102:443   *
                  facebook.com     1.845 ms     4.250 ms     4.722 ms   1:4 1260:4512 (3.6x)   289   3245     v1        31.13.66.35:443   !
                   youtube.com     2.702 ms     3.020 ms     6.491 ms   3:7 2520:8361 (3.3x)   288   6893     v1     142.251.163.93:443   *
                   twitter.com
                 instagram.com     0.944 ms     3.259 ms     3.717 ms   1:4 1260:4464 (3.5x)   290   3197     v1       31.13.66.174:443   !
```

### Full Help

```Bash
> quicreach --help
usage: quicreach <hostname(s)> [options...]
 -a, --alpn <alpn>      The ALPN to use for the handshake (def=h3)
 -b, --built-in-val     Use built-in TLS validation logic
 -c, --csv <file>       Writes CSV results to the given file
 -h, --help             Prints this help text
 -i, --ip <address>     The IP address to use
 -l, --parallel <num>   The numer of parallel hosts to test at once (def=1)
 -m, --mtu <mtu>        The initial (IPv6) MTU to use (def=1288)
 -p, --port <port>      The UDP port to use (def=443)
 -r, --req-all          Require all hostnames to succeed
 -s, --stats            Print connection statistics
 -u, --unsecure         Allows unsecure connections
 -v, --version          Prints out the version
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
