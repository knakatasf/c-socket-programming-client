![image](https://github.com/user-attachments/assets/6b1b9e61-7747-49c9-8eb4-590eecf7b936)

# C Socket Programming - Compression Detection Application

## **1. Developer Information**
**Name:** Koichi Nakata

## **2. Project Overview**
This project implements two network applications to detect network compression on a network path and locate the compression link.

### **2.1. Client-Server Application**
Requires cooperation between a client and a server. The application operates in three phases to detect compression along the network path.

#### **2.1.1. Pre-Probing Phase**
- The client establishes a TCP connection with the server.
- Configuration details are sent to the server.
- The TCP connection is closed after transmission.

#### **2.1.2. Probing Phase**
- The client sends two sets of UDP packet trains:
    - **Low-entropy packets** (zero-packet train).
    - **High-entropy packets** (random-packet train).
- The server records the arrival times of the first and last packets.

#### **2.1.3. Post-Probing Phase**
- The client establishes another TCP connection to receive the results.
- The server calculates the time difference between the two packet trains:
    - If the difference exceeds **100 ms**, the program outputs:
      ```
      Compression detected!
      ```
    - Otherwise, it outputs:
      ```
      No compression was detected.
      ```

### **2.2. Standalone Application**
Works independently without requiring any specific software on the other end. This mode operates without a dedicated server. The applications send UDP packet trains with low- and high-entropy data and analyze arrival times to determine if compression is present. If the time difference is more than a fixed threshold (100 ms), it indicates there is a compression link on the path.

#### **2.2.1. Packet Transmission**
The application sends two sets of the following packets. The first UDP packet train contains **Low-entropy packets** whereas the second one includes **High-entropy packets**.
- A **head SYN packet** is sent.
- A **UDP packet train** follows.
- A **tail SYN packet** is sent.

#### **2.2.2. Server Response (RST Packets)**
- SYN packets are sent to two different closed ports.
- The server responds with **RST packets**.

#### **2.2.3. Compression Detection**
- The detection method is the same as the Client-Server Application.
- If **RST packets are not received**, the program outputs:
  ```
  Failed to detect due to insufficient information.
  ```
  
## **3. Build Instructions**

### **3.1. Unpack the application**
The application zip file includes two project folders: **c-socket-programming-client** and **c-socket-programming-server**. Each application folder should be located in separate Ubuntu machines (i.e. having different IP addresses).

### **3.1. Prerequisites**
At each machine's terminal, move to the directory root.
```bash
cd <project-folder-path-for-client-application>
cd <project-folder-path-for-server-application>
```

Ensure you have the gcc compiler installed (version 11.4.0 or later). If it's not been installed yet, run this command:
```bash
sudo apt install gcc
```

Install the required library:
```bash
sudo apt-get install libcjson-dev
```
This command line will install cJSON.h in an appropriate place where the gcc compiler can automatically detect. 

### **3.2. Building the Program**
Navigate to the project directory and compile each component:

#### **3.2.1. Client-Server Application - Client**
```bash
gcc src/client.c src/client_func.c src/config.c -o client -lcjson
```

#### **3.2.2. Client-Server Application - Server**
```bash
gcc src/server.c src/server_func.c src/config.c -o server -lcjson
```

#### **3.2.3. Standalone Application - Client**
```bash
gcc src/standalone.c src/standalone_func.c src/config.c src/client_func.c -o standalone -lcjson
```

### **3.3. Configuration File**
Before running the program, at the client machine, copy the example configuration file:

```bash
cp config.example.json config.json
```

Modify `config.json` with the appropriate parameters. **Do not change the keys** in the JSON file.

#### **Configuration Parameters**
- **`pre_tcp_port`** *(Integer)* – [For Client-Server mode] Server's port used for the **pre-probing phase** receiving JSON data.
- **`post_tcp_port`** *(Integer)* – [For Client-Server mode] Server's Port used for the **post-probing phase** sends the result.
- **`client_udp_port`** *(Integer)* – [For Client-Server mode] Client's port for sending UDP packet trains.
- **`server_udp_port`** *(Integer)* – [For Client-Server mode] Server's port for receiving UDP packet trains.
- **`uncorp_tcp_head`** *(Integer)* – [For Standalone mode] Server's port for the **head SYN packet**, which should be closed.
- **`uncorp_tcp_tail`** *(Integer)* – [For Standalone mode] Server's port for the **tail SYN packet**, which should be closed.
- **`client_ip`** *(String)* – IP address of the *client*.
- **`server_ip`** *(String)* – IP address of the *server*.
- **`udp_payload_size`** *(Integer)* – Size of each UDP packet in bytes.
- **`inter_measurement_time`** *(Integer)* – Time (in seconds) the client waits between sending **zero-packet** and **random-packet** trains.
- **`num_of_udp_packets`** *(Integer)* – Number of packets in each train.
- **`ttl`** *(Integer)* – Time-to-Live (TTL) value for UDP packets.

**Note:** The program assumes the values are properly configured before running the program; otherwise, it will issue an error.

## **4. Run the Program**

#### **4.1. Client-Server Application**
1. First, start the server:
   ```bash
   ./server <post_tcp_port>
   ```
   `<post_tcp_port>` must be identical with `post_tcp_port` you specified in `config.json`.

2. Start the client:
   ```bash
   ./client config.json
   ```

#### **4.2. Standalone Application**
Run the standalone application with root privileges:

```bash
sudo ./standalone config.json
```

#### **4.3. Allocate enough memory to buffer**
Sometimes, the kernel buffer (socket buffer at the Network Layer) doesn't have enough buffer memory to receive all the arriving packets at the server side, resulting in dropping many packets.
In this case, run this command before running the application:
```bash
sudo sysctl -w net.core.rmem_max=<buffer size>
```
This command will temporarily enlarge the kernel buffer.

## **5. Reflection**
### 5.1. How IP and TCP/UDP headers work
Through this hands-on project, I gained a deeper understanding of IP and TCP/UDP headers, including their structure and functionality. In particular, configuring raw sockets required me to define each byte of the header fields manually. The most challenging and error-prone part was calculating the **checksum**.
An IP header’s checksum only verifies the header itself, whereas a TCP/UDP checksum includes not only its own header but also the source IP, destination IP, protocol, and length taken from the IP header, as well as the payload. To calculate this checksum, I needed to declare a pseudo-header and pseudo-datagram.

### 5.2. Buffers in different layers
Initially, my application was unable to receive all 12,000 UDP packets from the server. A significant number of packets were dropped upon receipt. To address this, I increased the buffer size for the **transport layer (TCP/UDP)** using the following command:
```bash
setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &receive_buffer_size, sizeof(receive_buffer_size));
```
However, even after increasing the socket buffer, my application still dropped many packets. I discovered that packets were also being dropped at the **network layer (IP)**. To mitigate this, I ran the following command before launching my application:
```bash
sudo sysctl -w net.core.rmem_max=some_value
```
After increasing the network layer’s buffer, my application was finally able to receive all 12,000 UDP packets. However, I noticed that Wireshark was still dropping some packets. This confused me, as I assumed Wireshark captured packets directly from the network layer’s kernel buffer. If the network layer’s buffer was now large enough, why was Wireshark still missing packets?
Upon researching the official Wireshark documentation, I learned that Wireshark retrieves packets from a **capture buffer** located at the network layer. When I examined this buffer within Wireshark’s UI, I found that its size was insufficient to store all incoming packets.
From this project, I literally realized that there DOES exist the distinct layers. To resolve a network-related problem in the future, I learned that it is crutial to identify the specific layer where the problem originates (Of course, sometimes multiple layers can simultaneously have problems).
