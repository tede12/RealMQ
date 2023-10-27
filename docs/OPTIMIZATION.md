# System Optimization for Enhanced Benchmark Accuracy

This guide delves into various tweaks and optimizations you can apply to your system to reduce background interference
and maximize the precision of your benchmarks. These adjustments are crucial for creating a more controlled environment
where tests can be more consistent and reflective of system changes. It's important to understand that these
modifications don't transform your setup into a real-time system, but they do sharpen the accuracy of performance
measurements.

## Terminating irqbalance

**Irqbalance** is a background service that distributes hardware interrupts across processors in a multi-core system.
While it generally improves system performance, it might introduce some variability in benchmarking results due to its
dynamic nature.

Disable this service with the following command:

```bash
sudo systemctl disable --now irqbalance
```

## Streamlining System Services

### Switching Off the Graphical User Interface

The Graphical User Interface (GUI) can consume significant system resources. For benchmarking purposes, it's often best
to disable it to prevent any potential interference.

Use the following command to access the configuration menu:

```bash
sudo systemctl disable --now gdm
```

### Deactivating Additional Services

In a typical setup, there might be various active services that aren't necessary for benchmarking tasks. For instance,
on our **Nvidia Jetson** system, services like `teamviewer` and `docker` were running and were not required for our
tests. Disable them using:

```bash
sudo systemctl disable --now teamviewerd docker
```

### Kernel Parameter Adjustment

The Linux kernel features a multitude of parameters that control various aspects of system operation. Fine-tuning these
can lead to a noticeable improvement in the consistency and precision of benchmark results.

To alter these settings, edit the bootloader configuration:

```bash
sudo nano /boot/extlinux/extlinux.conf
```

Include the parameters: `isolcpus=1-3 nohz_full=1-3 mitigations=off`. Reboot the system for the changes to take effect.

## Persistent Commands for Boot Sequence

Certain commands should be run at every system start to maintain an optimized environment. Here’s a script that sets
various system parameters:

```bash
sudo -i 
sysctl vm.stat_interval=120
echo never > /sys/kernel/mm/transparent_hugepage/enabled
swapoff -a
sysctl -w net.ipv4.udp_mem="102400 873800 16777216"
sysctl -w net.core.netdev_max_backlog="30000"
sysctl -w net.core.rmem_max="67108864"
sysctl -w net.core.wmem_max="67108864"
sysctl -w net.core.rmem_default="67108864"
sysctl -w net.core.wmem_default="67108864"
```