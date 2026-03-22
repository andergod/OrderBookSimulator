# OrderBookSimulator

A high‑performance C++ project that implements a full **Level 3 order book simulator** and extends it into a real‑time market‑data processing pipeline. The system can:

- Process **limit orders**, **cancellations**, and **matches**
- Maintain a **Level 3 intrusive order book** with stable pointers
- Connect to a real exchange via **secure WebSocket**
- Build custom **bars** from tick‑level data
- Store results efficiently using the **Parquet** columnar format

This project serves as both a learning tool and a foundation for real‑time trading research.

---

## 📦 Project Overview

### 1. Level 3 Order Book Simulator
The core component is a Level 3 order book that tracks every individual order.  
It supports:

- Adding limit orders  
- Cancelling orders  
- Matching orders  
- Printing or exporting the book state  

The order book uses **intrusive linked lists** for maximum performance and pointer stability.

### 2. Real‑Time Exchange Connection
Building on the simulator, the project includes a WebSocket client that:

- Connects securely using **Boost.Beast**
- Streams live market data
- Stores orders using the same intrusive structure
- Feeds updates into the bar‑building engine

### 3. Bar Construction & Parquet Output
Incoming ticks are aggregated into bars (time‑based for now).  
Bars are written to **Parquet** using:

- Arrow `RecordBatch` creation  
- Snappy compression  
- Optional row‑group partitioning  

This produces compact, analytics‑friendly files.

---

## 🛠️ Dependencies & Setup

This project uses **vcpkg** as the package manager.

### Dependencies
- Boost (Beast, Asio, etc.)
- Arrow / Parquet
- nlohmann/json (included as a git submodule)

### Clone the repository
```bash
git clone <your-repo-url>
cd OrderBookSimulator
git submodule update --init --recursive