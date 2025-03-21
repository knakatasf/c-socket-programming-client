# C Socket Programming - Compression Detection Application

## **1. Developer Information**
**Name:** Koichi Nakata

## **2. Project Overview**
This project implements two network applications to detect network compression on a network path and locate the compression link:

### **2.1. Client-Server Application**
- Requires cooperation between a client and a server.

### **2.2. Standalone Application**
- Works independently without requiring any specific software on the other end.

The applications send UDP packet trains with low- and high-entropy data and analyze arrival times to determine if compression is present. If the time difference is more than a fixed threshold (100 ms), it indicates there is a compression link on the path.

## **3. Build and Run Instructions**

### **3.1. Prerequisites**
Assume the user's machine is running Ubuntu. Before building the program, install the required library:

```bash
sudo apt-get install libcjson-dev
```

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

#### **3.2.3. Standalone Application**
```bash
gcc src/standalone.c src/standalone_func.c src/config.c src/client_func.c -o standalone -lcjson
```

### **3.3. Configuration File Setup**
Before running the program, copy the example configuration file:

```bash
cp config.example.json config.json
```

Modify `config.json` with the appropriate parameters. **Do not change the keys** in the JSON file.

#### **Configuration Parameters**
- **`pre_tcp_port`** *(Integer)* – Port used for the **pre-probing phase** (sends JSON data to the server).
- **`post_tcp_port`** *(Integer)* – Port used for the **post-probing phase** (sends results to the client).
- **`client_udp_port`** *(Integer)* – Client's port for sending UDP packet trains.
- **`server_udp_port`** *(Integer)* – Server's port for receiving UDP packet trains.
- **`uncorp_tcp_head`** *(Integer)* – Client's port for sending the **head SYN packet** to the server.
- **`uncorp_tcp_tail`** *(Integer)* – Client's port for sending the **tail SYN packet** to the server.
- **`server_ip`** *(String)* – IP address of the server.
- **`udp_payload_size`** *(Integer)* – Size of each UDP packet in bytes.
- **`inter_measurement_time`** *(Integer)* – Time (in seconds) the client waits between sending **zero-packet** and **random-packet** trains.
- **`num_of_udp_packets`** *(Integer)* – Number of packets in each train.
- **`ttl`** *(Integer)* – Time-to-Live (TTL) value for UDP packets.

**Note:** The program assumes the values are properly configured before running the program; otherwise, it will issue an error.

### **3.4. Running the Program**

#### **3.4.1. Client-Server Application**
1. Start the server:
   ```bash
   ./server <post_tcp_port specified in config.json>
   ```
2. Start the client:
   ```bash
   ./client config.json
   ```

#### **3.4.2. Standalone Application**
Run the standalone application with root privileges:

```bash
sudo ./standalone config.json
```

## **4. How the Application Works**

### **4.1. Client-Server Application**
The application operates in three phases to detect compression along the network path.

#### **4.1.1. Pre-Probing Phase**
- The client establishes a TCP connection with the server.
- Configuration details are sent to the server.
- The TCP connection is closed after transmission.

#### **4.1.2. Probing Phase**
- The client sends two sets of UDP packet trains:
  - **Low-entropy packets** (zero-packet train).
  - **High-entropy packets** (random-packet train).
- The server records the arrival times of the first and last packets.

#### **4.1.3. Post-Probing Phase**
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

### **4.2. Standalone Application**
This mode operates without a dedicated server.

#### **4.2.1. Packet Transmission**
- A **head SYN packet** is sent.
- A **UDP packet train** follows.
- A **tail SYN packet** is sent.

#### **4.2.2. Server Response (RST Packets)**
- SYN packets are sent to two different closed ports.
- The server responds with **RST packets** (Reset packets).
- If compression exists, the timing difference between the received RST packets will indicate it.

#### **4.2.3. Compression Detection**
- The detection method is the same as the Client-Server Application.
- If **RST packets are not received**, the program outputs:
  ```
  Failed to detect due to insufficient information.
  ```