# ShortX — Simple C++ URL Shortener + Load Balancer Simulator

ShortX is a high-performance, interview-ready portfolio project showcasing a custom **URL-shortening micro-service and Load Balancer Simulator** built in **C++17** with SQLite persistence, custom LRU caching, thread pool concurrency, asynchronous click analytics, and a lightweight React management dashboard.

---

## 📌 Features

* **C++17 High Performance Backend**: Built using standard STL abstractions and single-header libraries (`cpp-httplib`, `nlohmann/json`, `sqlite3`).
* **Base62 Short Code Encoding**: Converts unique 64-bit integer IDs to clean alphanumeric short codes (`[0-9a-zA-Z]`).
* **Custom LRU Cache**: Implemented using `std::unordered_map` and doubly-linked `std::list` for $O(1)$ lookup, recency promotion, and eviction tracking.
* **Custom C++ Thread Pool**: Fixed-worker producer-consumer pool (`std::vector<std::thread>`, `std::queue`, `std::condition_variable`).
* **Asynchronous Click Analytics**: Non-blocking click event processing queue decoupled from client 302 redirects.
* **Load Balancer Simulation**: In-app simulator supporting **Round Robin**, **Least Connections**, and **Consistent Hashing** with node failure & restoration mechanics.
* **Modern React Dashboard**: Built with Vite + React + Plain CSS for URL management, analytics inspection, and load balancer testing.
* **Comprehensive Test & Benchmark Suite**: 28 automated unit tests and high-precision micro-benchmarks measuring throughput, latency, and cache speedups.

---

## 🏗️ Architecture

```text
                  React Dashboard (Port 3000)
                        │
                  HTTP / REST API
                        │
            C++ HTTP Server (Port 8080)
                        │
        ┌───────────────┼───────────────┐
        │               │               │
    URL Service     LRU Cache      Load Balancer Simulator
        │                               │
        ▼                        ┌──────┼──────┐
     SQLite                  Server 1 Server 2 Server 3
        ▲
        │
 Analytics Queue
        │
 Background Worker
```

### Flow Breakdown:
1. **Shorten Request (`POST /api/urls`)**: Validates input URL, assigns auto-increment ID from SQLite (or custom alias), encodes ID into Base62, and returns short URL.
2. **Redirect Request (`GET /:shortCode`)**:
   - **Cache HIT**: Checked first in `LRUCache`. If found, immediately returns HTTP 302 redirect.
   - **Cache MISS**: Queries SQLite database, populates `LRUCache`, and returns HTTP 302 redirect.
   - **Async Analytics**: Pushes `ClickEvent` to non-blocking `AnalyticsQueue` worker thread, incrementing click counters in SQLite without delaying the client's redirect response.
3. **Load Balancer Simulation (`POST /api/load-balancer/simulate`)**: Simulates 1,000+ requests across virtual server nodes using selected routing algorithm and live node health states.

---

## 🛠️ Technology Stack

* **Backend**: C++17, `cpp-httplib` (HTTP), `sqlite3` (Database), `nlohmann/json` (JSON).
* **Frontend**: React 18, Vite, Plain CSS (Glassmorphic aesthetics), Lucide React.
* **Build System**: CMake 3.14+, MSVC / GCC / Clang.
* **Testing & Benchmarks**: Custom C++ assertion harness and high-resolution timer benchmark runner.

---

## 🚀 How to Build and Run

### Prerequisites
* CMake 3.14 or higher
* C++17 compatible compiler (MSVC 2022, GCC, or Clang)
* Node.js v18+ & npm

### 1. Build C++ Backend & Tests
```bash
# Configure build
cmake -B build -S .

# Compile release binaries
cmake --build build --config Release
```

The compiled executables will be produced in:
* Server: `build/backend/Release/shortx_server.exe`
* Unit Tests: `build/tests/Release/unit_tests.exe`
* Benchmarks: `build/benchmarks/Release/benchmark_suite.exe`

### 2. Run Backend Server
```bash
# Start C++ HTTP Backend on port 8080
./build/backend/Release/shortx_server.exe 8080
```

### 3. Run Frontend Dashboard
```bash
cd frontend
npm install
npm run dev
```
Open your browser at `http://localhost:3000`.

---

## ⚡ Unit Tests & Benchmark Results

### Running Unit Tests
```bash
./build/tests/Release/unit_tests.exe
```
Output:
```text
==========================================
       ShortX Unit Test Suite             
==========================================
--- Testing Base62Encoder ---
  [PASS] encode(1) == '1'
  [PASS] encode(62) == '10'
  [PASS] encode(3844) == '100'
  ...
==========================================
 Summary: 28 / 28 tests PASSED.
==========================================
```

### Empirical Benchmark Results
```bash
./build/benchmarks/Release/benchmark_suite.exe
```

#### 1. LRU Cache vs SQLite Lookup (10,000 Iterations)
| Storage Engine | Total Time (ms) | Avg Latency ($\mu s$) | Speedup Factor |
| :--- | :--- | :--- | :--- |
| **SQLite Database** | 47.38 ms | 4.74 $\mu s$ | Baseline |
| **Custom LRU Cache** | 0.45 ms | 0.045 $\mu s$ | **104.8x faster** |

#### 2. Thread Pool Scaling (1,000 Tasks)
| Worker Threads | Execution Time (ms) | Speedup |
| :--- | :--- | :--- |
| 1 Worker | 2.17 ms | 1.0x |
| 2 Workers | 0.93 ms | 2.33x |
| 4 Workers | 0.69 ms | **3.14x** |
| 8 Workers | 0.82 ms | 2.65x (overhead bound) |

#### 3. Load Balancer Throughput (100,000 Requests)
| Routing Algorithm | Duration (ms) | Simulated Throughput |
| :--- | :--- | :--- |
| **Round Robin** | 18.56 ms | ~5.38 Million req/sec |
| **Least Connections** | 17.82 ms | **~5.61 Million req/sec** |
| **Consistent Hashing** | 20.98 ms | ~4.77 Million req/sec |

---

## 📡 REST API Documentation

| Method | Endpoint | Description | Request / Response Sample |
| :--- | :--- | :--- | :--- |
| `POST` | `/api/urls` | Create short URL | `{"url": "https://github.com", "customAlias": "gh"}` |
| `GET` | `/api/urls` | List all active URLs | `{"urls": [...]}` |
| `GET` | `/api/urls/:code` | Get URL details | `{"shortCode": "gh", "originalUrl": "..."}` |
| `DELETE` | `/api/urls/:code` | Soft-delete URL | `{"success": true}` |
| `GET` | `/:code` | Redirect HTTP 302 | Header `Location: https://github.com` |
| `GET` | `/api/metrics` | System & Cache metrics | `{"cacheHits": 1712, "cacheHitRate": 98.2}` |
| `GET` | `/api/urls/:code/analytics` | Get click analytics | `{"totalClicks": 1829, ...}` |
| `GET` | `/api/load-balancer` | Get LB state & servers | `{"algorithm": "ROUND_ROBIN", "servers": [...]}` |
| `POST` | `/api/load-balancer/algorithm` | Change LB algorithm | `{"algorithm": "LEAST_CONNECTIONS"}` |
| `POST` | `/api/servers` | Add simulated server | `{"id": 4, "healthy": true, ...}` |
| `POST` | `/api/servers/:id/kill` | Kill simulated server | `{"success": true}` |
| `POST` | `/api/servers/:id/restore` | Restore server node | `{"success": true}` |
| `POST` | `/api/load-balancer/simulate` | Run request simulation | `{"count": 1000}` |

---

## 🔑 Core Class Explanations for Technical Interviews

1. **`Base62Encoder`**: Converts auto-incremented integer primary keys to Base62 strings using remainder division over character set `[0-9a-zA-Z]`. Guarantees collision-free short codes.
2. **`Database`**: Thin SQLite C-API wrapper managing `urls` and `click_events` tables with prepared statement binding and mutex-protected concurrency.
3. **`LRUCache`**: Thread-safe cache using `std::unordered_map<key, list_iterator>` and `std::list<pair<key, value>>`. Accessing items splices nodes to front ($O(1)$ recency promotion); exceeding capacity pops nodes from back ($O(1)$ eviction).
4. **`ThreadPool`**: Standard producer-consumer queue initialized with fixed worker threads. Worker loops block on `std::condition_variable` until tasks are enqueued.
5. **`AnalyticsQueue`**: Asynchronous task buffer separating redirect HTTP response generation from disk/DB click recording.
6. **`LoadBalancer`**: Manages collection of `SimulatedServer` instances. Implements Round Robin cyclic pointers, $O(N)$ Least Connections search, and a Virtual Node hash ring (`std::map<hash, serverId>`) for Consistent Hashing.

---

## 📄 License
MIT License. Created for technical interview demonstration.
