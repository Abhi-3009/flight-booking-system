# Flight Booking Management System

A high-concurrency, multi-role Flight Booking Management System built in C using a Client-Server TCP socket architecture. The system features raw file-based databases (`.csv`) with POSIX `fcntl` file locking and an industry-standard **Real-Time Seat-Level Locking & Reservation Engine with TTL (Temporary Holds)**.

---

## 🌟 Key Features

- **Concurrent Client-Server Networking:** Handles multiple simultaneous connections via multithreaded socket programming (`pthread`) and `poll()` based non-blocking I/O multiplexing.
- **Seat-Level Locking with Temporary Holds (TTL):** Passengers select specific seats (e.g. `1A`, `2B`) via a live visual seat map. The chosen seat is locked with a 120-second countdown, preventing race conditions and double bookings.
- **Auto-Release on Disconnect:** If a passenger drops connection or cancels checkout, any held seats and sessions are immediately released for other passengers.
- **Robust POSIX `fcntl` File Locking:** Multi-user data protection across flights, bookings, and customer databases.
- **Role-Based Access Control (RBAC):** Distinct dashboards and capabilities for Passengers, Agents, Managers, and Admins.
- **Duplicate Login & Deactivation Handling:** Prevents concurrent duplicate logins and enforces real-time account activation states.

---

## 👥 User Roles & Capabilities

### 🛫 Passenger (Customer)
- View available flights and interactive, real-time terminal seat maps.
- Lock and book specific flight seats securely with live hold timers.
- Cancel existing bookings (automatically releases seats back to the pool).
- Submit feedback.
- Change password.

### 👔 Agent (Employee)
- Add new passengers (with duplicate username prevention).
- Modify passenger details.
- View all flight schedules and passenger booking records.
- Change password.

### 📊 Manager
- Activate or deactivate passenger accounts.
- View and review passenger feedback records.
- View all system-wide booking transactions.
- Change password.

### 🛡️ System Admin
- Full system oversight.
- Add new flight routes (automatically generates seat layout matrix).
- Add new agents with uniqueness validation.
- Modify passenger and agent records.
- Change password.

---

## 🛠️ Installation & Setup

### Prerequisites
- GCC Compiler (C11 standard support)
- Make
- POSIX-compliant OS (macOS, Linux, BSD)
- Python 3 (for automated test suite)

### Build Instructions

1. **Initialize Database:**
   ```bash
   make setup
   ```

2. **Compile Binaries:**
   ```bash
   make all
   ```

3. **Run Automated Test Suite:**
   ```bash
   make test
   ```

---

## 🚀 Usage

Run the **Server** and **Client** in separate terminal windows:

**Terminal 1 (Server):**
```bash
./server
```

**Terminal 2 (Client):**
```bash
./client
```

### Default Test Credentials

| Role | Username | Password |
| :--- | :--- | :--- |
| **Passenger** | `customer1` | `pass123` |
| **Agent** | `employee1` | `pass123` |
| **Manager** | `manager1` | `pass123` |
| **Admin** | `admin1` | `pass123` |

---

## 📁 Project Structure

```text
├── Makefile                # Build scripts, test runners, and setup commands
├── server.c                # Multi-threaded server entry point
├── client.c                # Client entry point with poll() I/O
├── common.h                # Core structures and role constants
├── auth.c / auth.h         # Authentication and RBAC validation
├── session_manager.c/.h    # Thread-safe session tracking
├── seat_manager.c/.h       # Real-time seat map & temporary hold engine (TTL)
├── customer.c / .h         # Passenger workflows (seat selection, booking)
├── employee.c / .h         # Agent workflows (passenger management)
├── manager.c / .h          # Manager workflows (approvals, feedback)
├── admin.c / .h            # Admin workflows (flight creation, oversight)
├── utils.c / utils.h       # Shared utilities (string trimming, file locking)
├── test_system.py          # End-to-end automated seat-locking test suite
├── test_all_roles.py       # RBAC and role management test suite
└── data/                   # Database CSV files (flights, seats, bookings, users)
```
