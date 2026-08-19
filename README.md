# Flight Booking System

A robust, multi-role Flight Booking Management System built in C using a Client-Server TCP socket architecture. The system uses raw file-based databases (`.csv`) with explicit file-locking mechanisms to ensure data integrity during concurrent connections.

## Features

- **Concurrent Client-Server Architecture:** Handles multiple simultaneous users via multithreaded socket programming and `poll()` based I/O multiplexing.
- **Robust Data Integrity:** Implements rigorous POSIX `fcntl` file-locking to prevent race conditions when multiple users book flights concurrently.
- **Role-Based Access Control (RBAC):** Distinct dashboards and permissions for Passengers, Agents, Managers, and System Admins.
- **Custom Session Management:** Secure login handling and session tracking for all active users.

---

## User Roles & Capabilities

### 🛫 Passenger (Customer)
- View available flights and real-time seat availability.
- Book flight tickets securely.
- Cancel existing bookings (automatically updates available seats).
- Submit feedback.
- Manage profile (change password).

### 👔 Agent (Employee)
- Add new passengers.
- Modify passenger details.
- View assigned tasks and passenger queries.

### 📊 Manager
- Activate or deactivate passenger accounts.
- View and review passenger feedback.
- Oversee agent activities.

### 🛡️ System Admin
- Full system control.
- Add, modify, or remove Employees and Managers.
- Manage flight routes and global passenger data.

---

## Installation & Setup

### Prerequisites
- GCC Compiler
- Make
- Unix/Linux/macOS environment (POSIX compliant)

### Build Instructions

1. **Clone the repository**
   ```bash
   git clone https://github.com/Abhi-3009/flight-booking-system.git
   cd flight-booking-system
   ```

2. **Initialize Database**
   This sets up the `data/` directory with mock flights and user accounts.
   ```bash
   make setup
   ```

3. **Compile the Code**
   ```bash
   make all
   ```

---

## Usage

You need to run the **Server** and **Client** in two separate terminal windows.

**Terminal 1 (Server):**
```bash
./server
```

**Terminal 2 (Client):**
```bash
./client
```

### Default Test Credentials

Upon launching the client, you can log in using any of the default mock accounts:

| Role | Username | Password |
| :--- | :--- | :--- |
| **Passenger** | `customer1` | `pass123` |
| **Agent** | `employee1` | `pass123` |
| **Manager** | `manager1` | `pass123` |
| **Admin** | `admin1` | `pass123` |

---

## Technical Stack

- **Language:** C (C11 standard)
- **Networking:** BSD Sockets (TCP/IP)
- **Concurrency:** Pthreads & `poll()`
- **Storage:** CSV flat files with `fcntl` advisory locks.

## Project Structure

```text
├── Makefile                # Build scripts and setup commands
├── server.c                # Main server entry point & threading
├── client.c                # Main client entry point & polling
├── auth.c / auth.h         # Authentication and parsing logic
├── session_manager.c       # Active session tracking
├── customer.c              # Passenger logic (booking, viewing flights)
├── employee.c              # Agent logic (managing passengers)
├── manager.c               # Manager logic (approvals, feedback)
├── admin.c                 # Admin logic (system oversight)
├── utils.c / utils.h       # Shared utilities (string trimming, file locking)
└── data/                   # Generated database CSV files
```
