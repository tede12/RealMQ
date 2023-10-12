# Real-Time ZeroMQ Library


This project aims to transform the popular messaging library, [*ZeroMQ*](https://zeromq.org), into a real-time library suitable for applications
requiring guaranteed performance and low latencies. The primary focus is on the [pub/sub pattern](https://zguide.zeromq.org/docs/chapter5/) of *ZeroMQ*.

## Objectives

1. **Real-Time Transformation**: Modify the underlying architecture of ZeroMQ to make it suitable for real-time
   applications.
2. **Multi-threaded Testing**: Assess the performance and robustness of the library in a multi-threaded environment.
3. **Performance Benchmarking**: Compare the performance of the original ZeroMQ library with the real-time version using
   a series of benchmarks.

## Enhancements

Here are some of the enhancements made to the library:

- **UDP Communication Protocol**: Switched to UDP for a lighter, faster communication protocol suitable for real-time
  applications.
- **Quality of Service (QoS) Settings**: Implemented various QoS levels to evaluate their impact on latency, packet
  loss, and
  real-time communication reliability.
- **Dynamic Scaling**: Added the ability to dynamically scale the number of threads or processes depending on the
  machine's
  workload. This enhancement is critical in measuring its effect on performance and latency.

## Benchmarks

Performance is assessed using the following benchmarks:

1. **Latency**: A bar chart comparing latency under different workloads (e.g., messages per second). It's crucial to see
   how
   latency varies with workload intensity. In the context of real-time systems, understanding and optimizing latency is paramount for several reasons:
    
      - **Predictability and Consistency**: Real-time systems often require operations to be executed within a strict time frame. 
      High or variable latency can lead to unpredictability, which is unacceptable in environments where timing is critical, such as financial services or emergency response systems.
      - **Quality of Service (QoS)**: In messaging systems, the quality of service is often defined by how fast and reliably messages are delivered. 
      Latency metrics, especially the **standard deviation** of latency, provide insights into the network's performance consistency. A low standard deviation indicates a reliable system with generally predictable latency, whereas a high standard deviation might signal intermittent network issues or potential bottlenecks.
      - **Real-time Performance Insights**: The **95th percentile latency** is particularly important in real-time systems. It shows the maximum latency experienced by 95% of the messages, excluding outlying delays caused by exceptional conditions. This metric helps stakeholders understand the system's performance under normal conditions, ensuring that the vast majority of operations meet the required time constraints.
      By focusing on these latency-related metrics, stakeholders can make informed decisions about potential optimizations, understand the real-world performance of the system, and ensure that it meets the stringent requirements of real-time operations.

2. **Throughput**: A line graph illustrating messages handled per second by the original library vs. the real-time
   version over
   time.
3. **Jitter**: A measure of latency variability. A box plot is used to compare the jitter distribution between the two
   versions.
4. **Resource Usage**: Bar charts contrasting CPU, memory, and other resource usages between the original and real-time
   versions.
5. **Guaranteed Response Times**: In real-time environments, it's essential to know the percentage of cases where a
   message is
   processed within a specific time. Pie charts or histograms are used for visualization.
6. **Reliability Response**: Testing the robustness of the two versions under different stress conditions, visualized
   using bar
   or line charts.
7. **Cumulative Latency Graph**: This graph shows the percentage of requests completed within a particular time frame,
   helping
   to compare the real-time version's performance against the original for prolonged processing requests.

## Getting Started

### Prerequisites

- C compiler (e.g., gcc)
- Python (for generating graphs)

## Installation

1. Clone the repository

```bash
git clone https://www.github.com/tede12/RealMQ
```

2. Run the install script

```bash
chmod +x install.sh && ./install.sh
```

## Usage

1. Compile the project using CMake:

```bash
mkdir build && cd build && cmake .. && make
```

2. Run the server and client executables:

```bash
# client and server executables are located in the build directory

# Start the server node
./realmq_server

# (optional) run the plot.py script to graph the results
source ../venv/bin/activate && python3 ../plot.py

# Start the client node
./realmq_client
```
