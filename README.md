# iPerfer: Network Performance Measurement and Analysis Tool Development

A developed custom tool, iPerfer, to measure network bandwidth and latency by sending and receiving TCP packets between hosts. The tool uses sockets for TCP communication and is implemented in C/C++. Additionally, the project includes performance testing on virtual networks using Mininet simulating different network topologies.

## iPerfer

It has two modes:
<p>Server Mode: Listens for TCP connections, receives data, and calculates the received data rate.</p>
<p>Client Mode: Connects to a server, sends data for a specified duration, and calculates the sent data rate.</p>

### Server Mode

To run iPerfer in server mode:

```$ ./iPerfer -s -p <listen_port>```
<p>-s: Indicates server mode.</p>
<p>listen_port: Port number in the range [1024, 65535].</p>

### Client Mode

To run iPerfer in client mode:

```$ ./iPerfer -c -h <server_hostname> -p <server_port> -t <time>```
<p>-c: Indicates client mode.</p>
<p>server_hostname: Hostname or IP address of the server.</p>
<p>server_port: Port number in the range [1024, 65535].</p>
<p>time: Duration in seconds for data transmission.</p>

## Measurements in Mininet
Using iPerfer and the latency measurement tool ping, I measured bandwidth and latency in a Mininet virtual network. The measurements include:
- Link latency and throughput between switches.
- Path latency and throughput between hosts.
- Effects of multiplexing on network performance.

## Custom Topology
Custom topology in bdreyer_topology.py and it's visual representation in bdreyer_topology.png
