# ✈️ Flight Booking Management System

[![Language: C11](https://img.shields.io/badge/Language-C11-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform: POSIX](https://img.shields.io/badge/Platform-macOS%20%7C%20Linux-FF6F00?logo=linux&logoColor=white)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![Networking: BSD Sockets](https://img.shields.io/badge/Networking-TCP%2FIP%20Sockets-4B8BBE)](https://en.wikipedia.org/wiki/Berkeley_sockets)
[![Concurrency: Pthreads](https://img.shields.io/badge/Concurrency-POSIX%20Threads-brightgreen)](https://en.wikipedia.org/wiki/Pthreads)
[![Security: Bcrypt](https://img.shields.io/badge/Security-Bcrypt%20Salted%20Hashing-critical)](https://en.wikipedia.org/wiki/Bcrypt)

A high-performance, multi-tiered **Flight Booking Management System** built in **C (C11)** using **BSD TCP Sockets**, **POSIX Threads (`pthreads`)**, and **`poll()` I/O Multiplexing**. 

The system features an industry-standard **Real-Time Seat-Level Locking & Reservation Engine with Time-To-Live (TTL) Leases**, transactional flat-file storage with **POSIX `fcntl` advisory locks**, and **Bcrypt one-way salted password hashing** ($2a$10$).

---

## 📑 Table of Contents

- [System Architecture](#-system-architecture)
- [Key Features](#-key-features)
- [Role-Based Access Control (RBAC)](#-role-based-access-control-rbac)
- [Interactive Seat-Level Locking Engine](#-interactive-seat-level-locking-engine)
- [Cryptographic Security (Bcrypt)](#-cryptographic-security-bcrypt)
- [Installation & Quick Start](#-installation--quick-start)
- [Automated Testing Suite](#-automated-testing-suite)
- [Project Directory Structure](#-project-directory-structure)

---

## 🏛 System Architecture

```mermaid
graph TD
    subgraph TIER1["1. CLIENT TIER (poll() Non-Blocking I/O)"]
        C1["Passenger Client"]
        C2["Agent Client"]
        C3["Admin / Manager Client"]
    end

    subgraph TIER2["2. TCP NETWORKING TIER"]
        SOCK["Server Master Socket (Port 9091)"]
        TP["Worker Thread Pool (POSIX Threads)"]
    end

    subgraph TIER3["3. CONCURRENCY & LOGIC ENGINES"]
        SM["Session Manager<br/>(pthread_mutex_t)"]
        SL["Seat Hold Engine<br/>(TTL Lease Lock)"]
        BC["Bcrypt Crypto Engine<br/>($2a$10$ Salting)"]
    end

    subgraph TIER4["4. PERSISTENT STORAGE (POSIX fcntl File Locks)"]
        D1[("flights.csv & seats.csv")]
        D2[("bookings.csv")]
        D3[("user credentials (*.csv)")]
    end

    C1 & C2 & C3 -->|"TCP Sockets"| SOCK
    SOCK -->|"pthread_create()"| TP
    TP <--> SM
    TP <--> SL
    TP <--> BC
    TP <-->|"fcntl(F_WRLCK)"| D1 & D2 & D3
```

---

## 🌟 Key Features

- **Concurrent Multi-Client Server:** Multithreaded client handling (`pthread_create`, `pthread_detach`) paired with `poll()` based non-blocking I/O multiplexing.
- **Real-Time Seat-Level Locking (Lease with TTL):** Passengers select individual seats (`1A`, `2B`, etc.) on a live visual seat map. Selected seats are temporarily held exclusively via an optimistic lease lock with configurable time-to-live (TTL), preventing race conditions and double bookings.
- **Fault-Tolerant Auto-Release on Disconnect:** If a client crashes or abruptly terminates during checkout, the server catches the disconnect, avoids `SIGPIPE` crashes, immediately frees all active seat holds, and clears the session.
- **Bcrypt Salted Password Hashing:** Replaces plaintext credential storage with self-contained **EksBlowfish Bcrypt** password hashing, generating unique 128-bit random salts per user with constant-time verification.
- **POSIX `fcntl` Concurrency Control:** Flat-file database transactions are protected with file locking and thread mutexes, guaranteeing ACID-like consistency without race conditions.
- **Zero Duplicate Logins:** Real-time session manager prevents simultaneous duplicate logins from multiple endpoints for the same user account.

---

## 👥 Role-Based Access Control (RBAC)

| Role | Primary Responsibilities & Key Permissions |
| :--- | :--- |
| **🛫 Passenger** | Browse flights & seat maps, reserve & lock seats (TTL lease), book/cancel tickets, submit feedback |
| **👔 Agent** | Create passenger profiles with uniqueness checks, modify passenger details, view booking logs |
| **📊 Manager** | Activate/deactivate passenger accounts, view comprehensive bookings, review customer feedback |
| **🛡️ Admin** | Create flight routes & initialize seat matrices, add/manage agents, full system oversight |

---

## 💺 Interactive Seat-Level Locking Engine

When a passenger initiates a booking, the server renders a terminal visual seat map and begins an exclusive lease:

```text
======================= FLIGHT 1 SEAT MAP =======================
  Col A      Col B          Col C      Col D
---------------------------------------------------------------
[1A:FREE]  [1B:FREE]      [1C:FREE]  [1D:TAKEN] 
[2A:FREE]  [2B:TAKEN]     [2C:HELD]  [2D:FREE]  
[3A:FREE]  [3B:FREE]      [3C:FREE]  [3D:FREE]  
[4A:FREE]  [4B:FREE]      [4C:FREE]  [4D:FREE]  
[5A:FREE]  [5B:FREE]      [5C:FREE]  [5D:FREE]  
---------------------------------------------------------------
Legend: [FREE] = Available | [HELD] = Reserved (Hold Active) | [TAKEN] = Booked
===============================================================
```

### Seat State Machine:
```mermaid
flowchart TD
    INIT([Start]) --> S1["🟢 AVAILABLE<br/>(Seat is open for booking)"]
    
    S1 -->|"1. Passenger reserves seat"| S2["🟡 HELD<br/>(Exclusive Hold Lease)"]
    
    S2 -->|"2a. Confirms Booking ('Y')"| S3["🔴 BOOKED<br/>(Ticket Confirmed)"]
    
    S2 -->|"2b. Cancel ('N') / Lease Expired / Disconnect"| S1
    
    S3 -->|"3. Passenger Cancels Ticket"| S1
```

---

## 🔒 Cryptographic Security (Bcrypt)

All user credentials across `customers.csv`, `employees.csv`, `managers.csv`, and `admins.csv` are protected using **Bcrypt** ($2a$10$):

```csv
id,username,password,active
1,customer1,$2a$10$B4Zf7oMrDsAHLE1s4u8/IukCbtq2GujtNF3uxruThMYKwtg2V8ey.,1
2,customer2,$2a$10$B4Zf7oMrDsAHLE1s4u8/IukCbtq2GujtNF3uxruThMYKwtg2V8ey.,1
3,alice,$2a$10$4WjDR2k2gsmeQSA5QUbTVeJ1bOldzncYurx63wiiMGQpjY3XJTsgC,1
```

- **Salt Generation:** Unique 128-bit cryptographically secure pseudorandom salt per record from `/dev/urandom`.
- **Work Factor (Cost):** $2^{10} = 1024$ key expansion rounds to defeat GPU/ASIC brute-force and dictionary attacks.
- **Timing-Attack Resistance:** Employs constant-time verification in `bcrypt_checkpw()`.

---

## 🚀 Installation & Quick Start

### Prerequisites
- **Compiler:** GCC or Clang (C11 support)
- **Build Tool:** Make
- **Environment:** POSIX-compliant OS (macOS, Linux, BSD)
- **Testing:** Python 3 (for automated test suites)

### 1. Clone & Build

```bash
git clone https://github.com/Abhi-3009/flight-booking-system.git
cd flight-booking-system
```

### 2. Initialize Database

Seeds mock flights, users, and initializes bcrypt password hashes:
```bash
make setup
```

### 3. Compile Binaries

```bash
make all
```

### 4. Run Server & Client

Open two separate terminal windows:

**Terminal 1 (Server):**
```bash
./server
```

**Terminal 2 (Client):**
```bash
./client
```

---

## 🔑 Default Test Credentials

| Role | Username | Plaintext Password |
| :--- | :--- | :--- |
| **Passenger** | `customer1` *(or `customer2`, `alice`, `bob`)* | `pass123` *(or `alice123`, `bob123`)* |
| **Agent** | `employee1` | `pass123` |
| **Manager** | `manager1` | `pass123` |
| **Admin** | `admin1` | `pass123` |

---

## 🧪 Automated Testing Suite

The repository includes comprehensive automated concurrency and RBAC test suites in Python.

Run all tests with a single command:

```bash
make test
```

### Test Coverage Summary:
- ✅ **Concurrent Seat Locking:** Verifies dual-client simultaneous seat selection and lock contention.
- ✅ **Hold Expiration & Cancellation:** Verifies timeout release and explicit cancellation rollback.
- ✅ **Disconnect Recovery:** Verifies instant hold release upon sudden client socket termination.
- ✅ **Booking Cancellation:** Verifies seat restoration and flight capacity increment.
- ✅ **Bcrypt Authentication:** Verifies password hashing, authentication, and password updates.
- ✅ **RBAC Operations:** Verifies duplicate username prevention, account deactivation, and admin oversight.

---

## 📁 Project Directory Structure

```text
.
├── Makefile                # Build automation, database seeding, and test targets
├── server.c                # Multi-threaded TCP server & connection handling
├── client.c                # Client entry point with poll() I/O multiplexing
├── common.h                # Core structs, constants, and role definitions
├── auth.c / auth.h         # Bcrypt authentication and session registration
├── session_manager.c / .h  # Thread-safe session tracking (duplicate login prevention)
├── seat_manager.c / .h     # Real-time seat map & temporary hold engine (TTL lease)
├── bcrypt.c / bcrypt.h     # Standalone EksBlowfish bcrypt cryptographic engine
├── init_db.c               # Database initialization and bcrypt seed generator
├── customer.c / .h         # Passenger workflows (seat selection, booking, cancel)
├── employee.c / .h         # Agent workflows (passenger creation, modification)
├── manager.c / .h          # Manager workflows (approvals, feedback review)
├── admin.c / .h            # Admin workflows (flight creation, oversight)
├── utils.c / utils.h       # Shared utilities (string trimming, file locking)
├── test_system.py          # End-to-end automated seat-locking test suite
├── test_all_roles.py       # Role management & RBAC test suite
└── data/                   # Flat-file databases (flights, seats, bookings, users)
```
