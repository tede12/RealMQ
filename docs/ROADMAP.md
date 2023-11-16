# RealMQ - Roadmap

This document describes the current roadmap for RealMQ. It is a living document and will be updated as the project

*Updated: Fri, 17 Nov 2023*

## UDP Enhancement

#### Milestone Summary

| Status | Milestone                                                                                   | Goals | ETA |
|:------:|:--------------------------------------------------------------------------------------------|:-----:|:---:|
|   🚀   | **[UDP Packet Loss Detection and Enhancement](#udp-packet-loss-detection-and-enhancement)** | 1 / 5 | TBD |
|   🚀   | **[Memory Cleanup Bug Investigation](#memory-cleanup-bug-investigation)**                   | 1 / 5 | TBD |

#### UDP Packet Loss Detection and Enhancement

> Addressing packet loss in the UDP protocol and enhancing its efficiency.

🚀 &nbsp;**OPEN** &nbsp;&nbsp;📉 &nbsp;&nbsp;**0 / 5** goals completed **(0%)** &nbsp;&nbsp;📅 &nbsp;&nbsp;**TBD**

| Status | Goal                                                                             | Labels        | Branch                                |
|:------:|:---------------------------------------------------------------------------------|---------------|---------------------------------------|
|   ❌    | Implement mechanism for UDP to recognize lost packets                            | `in progress` | <a href=#>udp/packet-detection</a>    |
|   ❌    | Modify message IDs and implement a method for space optimization                 | `ready`       | <a href=#>udp/msg-id-optimization</a> |
|   ✅    | Implement search interpolation log log n for lost message detection              | `done`        | <a href=#>udp/packet-detection</a>    |
|   ❌    | Implement mechanism to resend lost packets                                       | `in progress` | <a href=#>udp/packet-resend</a>       |
|   ❌    | Determine the correct threshold for the accrual detector for heartbeat frequency | `ready`       | <a href=#>udp/heartbeat-frequency</a> |

#### Memory Cleanup Bug Investigation

> Investigate the reason why memory is not being cleared.

🚀 &nbsp;**OPEN** &nbsp;&nbsp;📉 &nbsp;&nbsp;**1 / 5** goals completed **(0%)** &nbsp;&nbsp;📅 &nbsp;&nbsp;**TBD**

| Status | Goal                                                     | Labels        | Branch                              |
|:------:|:---------------------------------------------------------|---------------|-------------------------------------|
|   ✅    | Memory leak in config.c and logger.c                     | `done`        | <a href=#>bugfix/memory-cleanup</a> |
|   ❌    | Memory leak in Json handler in src/realmq_server.c       | `in progress` | <a href=#>bugfix/memory-cleanup</a> |
|   ❌    | Memory leak in server_thread in src/realmq_server.c      | `ready`       | <a href=#>bugfix/memory-cleanup</a> |
|   ❌    | Memory leak in save_stats_to_file in src/realmq_server.c | `ready`       | <a href=#>bugfix/memory-cleanup</a> |
|   ❌    | Memory leak in client_thread in src/realmq_client.c      | `ready`       | <a href=#>bugfix/memory-cleanup</a> |
