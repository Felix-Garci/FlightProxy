# **FlightProxy Middleware**

**FlightProxy** is a modern C++ (C++17) middleware architecture designed for critical communication systems in unmanned vehicles. Its core philosophy is **"Write Once, Run Anywhere"**, allowing the same business logic to run on both embedded hardware (ESP32/FreeRTOS) and desktop environments (Windows) for simulation and validation.

## **🎯 Project Philosophy**

This project addresses the complexity of embedded software through a strict separation of concerns:

1. **Total Hardware Abstraction (HAL & OSAL):** The application code is agnostic to the underlying Operating System.  
2. **Transport Agnostic:** Control logic does not know (and does not care) if data travels via WiFi (TCP/UDP) or cable (UART).  
3. **Simulation-First:** The architecture allows testing complex protocols and state machines on the PC before touching real hardware.

## **🏗️ Transport Architecture (Deep Dive)**

The heart of FlightProxy's portability lies in how it implements the ITransport interface. While the Core only sees a generic contract (open, send, close), each platform provides radically different implementations adapted to its context:

### **🔌 PlatformESP32: "Metal & Silicon"**

Designed for production deployment. Here, implementations are efficient wrappers over the hardware and Espressif SDK:

* **SimpleTCP / SimpleUDP (LwIP):** Implement network transport using the **LwIP** (Lightweight IP) stack. They manage the lifecycle of BSD Sockets and use dedicated FreeRTOS tasks for asynchronous event handling without blocking the main loop.  
* **SimpleUart (Hardware Driver):** Interacts directly with the chip's UART peripheral using the **ESP-IDF** driver. It handles interrupts, circular buffers (Ring Buffers), and hardware events (like FIFO overflow) to ensure minimal latency.

### **💻 PlatformWin: "Virtualization & Mocking"**

Designed for agile development and testing. Here, implementations aim to facilitate debugging:

* **SimpleTCP / SimpleUDP (WinSock2):** Map calls to the Windows Network API (**WinSock2**). This allows the PC to simulate being the Drone or Ground Station, interacting with real network tools.  
* **SimpleUart (Smart Mock):** *This is a key piece of the architecture.* Instead of tying to a physical COM port, this implementation acts as a **Virtual Loopback**.  
  * **Behavior:** Any data "sent" is immediately re-injected as "received" data.  
  * **Value:** Allows validation of the entire processing chain (Encoders, Decoders, State Machines, and Channel Logic) in isolation, without external hardware or cables.

## **🧩 System Components**

### **1\. Core (lib/Core)**

Defines contracts and base types. It is pure C++ and has no external dependencies.

* **OSAL:** Interfaces for ITask, IMutex, IQueue.  
* **Transport Interface:** Contracts for ITransport (Send/Receive) and ITcpListener.  
* **Protocol:** Generic IEncoderT and IDecoderT interfaces (Current support: MSP V2 and IBUS).

### **2\. Channel Middleware (lib/Channel)**

A higher-level abstraction layer that endows raw transports with "intelligence":

* **ChannelT:** The basic unit. Links an ITransport with its corresponding codec.  
* **ChannelPersistentT:** A decorator that endows any channel with **auto-recovery**. If the TCP connection drops or the UART driver fails, this module manages automatic reconnection in the background.  
* **ChannelAgregator / Disgregator:** Multiplexers that allow routing multiple logical command streams over a single physical link.

## **🚀 Development Environments (PlatformIO)**

The project uses platformio.ini to orchestrate cross-compilation:

| Environment | Target | Description |
| :---- | :---- | :---- |
| **env:esp32-idf** | **ESP32** | Compiles against **ESP-IDF** and **FreeRTOS**. Generates the final binary for the vehicle. |
| **env:native** | **Windows** | Compiles a native .exe. Links against **Win32 API** and **STL Threads**. Ideal for Unit Testing and logical simulation. |

## **📂 Repository Structure**
```
├── lib/  
│   ├── Core/             # Public interfaces (Contracts)  
│   ├── Channel/          # Middleware Logic (Persistence, Mux/Demux)  
│   ├── PlatformESP32/    # Real Implementation (LwIP, ESP-IDF UART)  
│   ├── PlatformWin/      # Simulation Implementation (WinSock, Mock UART)  
│   ├── Connectivity/     # High-level managers (e.g., WiFiManager)  
│   └── AppLogic/         # (WIP) Business Logic and Control  
├── src/  
│   └── main.cpp          # Dependency Injection (Composition Root)  
└── platformio.ini        # Build System Configuration
```

## **🛠️ Tech Stack**

* **Language:** C++17  
* **Build System:** PlatformIO  
* **RTOS:** FreeRTOS (ESP32) / std::thread (Win)  
* **Supported Protocols:** MSP V2, IBUS (FlySky), TCP/IP, UDP.

*Project Status: Transport infrastructure and middleware completed. Application Logic (AppLogic) layer under active development.*