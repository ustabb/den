+------------------------------------------------+
|                  Control & API Layer           |
|------------------------------------------------|
| - İzleyici sayısı kontrolü                     |
| - Gecikme / kalite ayarları                    |
| - Runtime config (JSON/TOML)                  |
| - REST/gRPC API                               |
+------------------------------------------------+
                   |
                   v
+------------------------------------------------+
|               Streaming Layer                  |
|------------------------------------------------|
| - Subscriber management                        |
| - Adaptive bitrate / congestion control        |
| - Protocol routing (RTMP / HLS / WebRTC)      |
+------------------------------------------------+
                   |
                   v
+------------------------------------------------+
|               Media Layer                      |
|------------------------------------------------|
| - Video/Audio Encoder / Decoder                |
| - Frame struct & timestamp management          |
| - Compression algorithms                        |
+------------------------------------------------+
                   |
                   v
+------------------------------------------------+
|               Network Layer                    |
|------------------------------------------------|
| - TCP/UDP/QUIC transport                       |
| - Low-latency packet handling                  |
| - Boost.Asio / libuv                            |
+------------------------------------------------+
                   |
                   v
+------------------------------------------------+
|                  Storage / VOD Layer          |
|------------------------------------------------|
| - Recording live streams                        |
| - Segmenting videos for HLS                     |
| - File system management                        |
+------------------------------------------------+


