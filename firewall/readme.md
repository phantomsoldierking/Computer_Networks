# 🧱 Kernel-Level Firewall Demo with TCP Client-Server

This project demonstrates how to:

- Build a **kernel-level firewall** using Linux Netfilter hooks.  
- Create a simple **TCP client-server** communication setup.  
- Block connections from a specific IP address **inside the kernel**, without using `iptables` or user-space tools.

---

## 📁 Project Structure

```

firewall-demo/
├── kernel_firewall.c   # Kernel module to drop packets from a given IP
├── server.c            # TCP server program
├── client.c            # TCP client program
├── Makefile            # Builds all components
└── README.md           # Documentation

````

---

## ⚙️ Build Instructions

Clone and build everything:

```bash
git clone <repo-url>
cd firewall-demo
make
````

This builds the following:

* `kernel_firewall.ko` → The kernel module (firewall)
* `server` → TCP server binary
* `client` → TCP client binary

---

## 🧱 Running the Demo

### 2️⃣ Start the Server

On **Machine A** (example IP = `192.168.1.10`):

```bash
./server
```

Expected output:

```
Server listening on port 8080...
```

---

### 3️⃣ Start the Client

On **Machine B** (can be another device on the same network):

```bash
./client 192.168.1.10
```

Expected output:

```
Message sent to server
Server replied: Hello from server
```

---

### 4️⃣ Load the Firewall Module

Now load the firewall module on the **server machine (Machine A)** to block the client IP.

1. Find the IP address of the client (e.g., `192.168.1.20`).

2. Convert the IP to hexadecimal (network byte order):

   ```
   192.168.1.20 → 0xC0A80114
   ```

3. Load the kernel firewall module with the blocked IP parameter:

   ```bash
   sudo insmod kernel_firewall.ko block_ip=0xC0A80114
   ```

4. Check kernel logs:

   ```bash
   dmesg | tail -n 5
   ```

   Expected output:

   ```
   Kernel Firewall loaded. Blocking IP: 192.168.1.20
   ```

---

### 5️⃣ Test the Firewall

Now, attempt to connect again from the client machine:

```bash
./client 192.168.1.10
```

Expected output:

```
Connection Failed: Connection timed out
```

Server kernel log (`dmesg` output):

```
KernelFirewall: Dropped packet from 192.168.1.20
```

The connection is dropped at the kernel level — the server process never receives the packet.

---

### 6️⃣ Remove the Module

When you’re done testing, unload the firewall module:

```bash
sudo rmmod kernel_firewall
dmesg | tail -n 5
```

Expected output:

```
Kernel Firewall unloaded.
```

Now, the client can connect again successfully.

---

## 🧠 How It Works

* The **Netfilter hook** intercepts all packets in the kernel at the `NF_INET_PRE_ROUTING` stage.
* Each incoming packet’s **source IP address** is checked.
* If it matches the blocked IP address, the packet is dropped (`NF_DROP`).
* Otherwise, it is accepted (`NF_ACCEPT`).
* Because this happens inside the kernel, the packet never reaches the user-space server if it’s dropped.

---

## 🧹 Cleanup

To clean all build artifacts:

```bash
make clean
```

---

## 🧩 Notes

* You need Linux kernel headers to build the module:

  ```bash
  sudo apt install linux-headers-$(uname -r)
  ```

  *(Only required during compilation, not at runtime.)*

* Use `sudo` when inserting or removing kernel modules:

  ```bash
  sudo insmod kernel_firewall.ko block_ip=0xC0A80114
  sudo rmmod kernel_firewall
  ```

* Tested on **Linux Kernel 5.x+** (should work on most modern distributions).

---