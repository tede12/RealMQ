# Benchmark Setup

## Hardware











```bash

taskset --cpu-list 1 ./realmq_server
taskset --cpu-list 2 ./realmq_client
```


```bash
tegrastats  --interval 800 --logfile ./tegrastats_udp_burst.log
```