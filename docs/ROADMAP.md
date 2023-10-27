# RealMQ - Roadmap

This document describes the current roadmap for RealMQ. It is a living document and will be updated as the project

*Updated: Fri, 27 Oct 2023*

## UDP Enhancement

#### Milestone Summary

| Status | Milestone                                                                                   | Goals | ETA |
|:------:|:--------------------------------------------------------------------------------------------|:-----:|:---:|
|   🚀   | **[UDP Packet Loss Detection and Enhancement](#udp-packet-loss-detection-and-enhancement)** | 0 / 5 | TBD |
|   🚀   | **[Memory Cleanup Bug Investigation](#memory-cleanup-bug-investigation)**                   | 0 / 1 | TBD |

#### UDP Packet Loss Detection and Enhancement

> Addressing packet loss in the UDP protocol and enhancing its efficiency.

🚀 &nbsp;**OPEN** &nbsp;&nbsp;📉 &nbsp;&nbsp;**0 / 5** goals completed **(0%)** &nbsp;&nbsp;📅 &nbsp;&nbsp;**TBD**

| Status | Goal                                                                             | Labels        | Branch                                        |
|:------:|:---------------------------------------------------------------------------------|---------------|-----------------------------------------------|
|   ❌    | Implement mechanism for UDP to recognize lost packets                            | `in progress` | <a href=#>feature/udp-packet-detection</a>    |
|   ❌    | Modify message IDs and implement a method for space optimization                 | `ready`       | <a href=#>feature/msg-id-optimization</a>     |
|   ❌    | Implement search interpolation log log n for lost message detection              | `ready`       | <a href=#>feature/interpolation-detection</a> |
|   ❌    | Implement mechanism to resend lost packets                                       | `ready`       | <a href=#>feature/udp-packet-resend</a>       |
|   ❌    | Determine the correct threshold for the accrual detector for heartbeat frequency | `ready`       | <a href=#>feature/heartbeat-frequency</a>     |

#### Memory Cleanup Bug Investigation

> Investigate the reason why memory is not being cleared.

🚀 &nbsp;**OPEN** &nbsp;&nbsp;📉 &nbsp;&nbsp;**0 / 1** goals completed **(0%)** &nbsp;&nbsp;📅 &nbsp;&nbsp;**TBD**

| Status | Goal                                                                | Labels  | Branch                              |
|:------:|:--------------------------------------------------------------------|---------|-------------------------------------|
|   ❌    | Investigate and identify the root cause of memory not being cleared | `ready` | <a href=#>bugfix/memory-cleanup</a> |
