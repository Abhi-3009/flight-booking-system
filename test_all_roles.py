import socket
import time
import subprocess
import os

def start_server():
    server_proc = subprocess.Popen(["./server"], cwd=os.getcwd(), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(50):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect(("127.0.0.1", 9091))
            s.close()
            break
        except Exception:
            time.sleep(0.1)
    return server_proc

def read_until(sock, delimiter, timeout=4.0):
    sock.settimeout(timeout)
    data = ""
    start = time.time()
    while time.time() - start < timeout:
        try:
            chunk = sock.recv(1024).decode('utf-8', errors='ignore')
            if not chunk:
                break
            data += chunk
            if delimiter in data:
                break
        except socket.timeout:
            break
    return data

def send_line(sock, text):
    sock.sendall((text + "\n").encode('utf-8'))
    time.sleep(0.1)

def run_role_tests():
    print("🚀 Starting RBAC & Role Management Test Suite...")
    subprocess.run(["make", "setup"], check=True)
    subprocess.run(["make", "rebuild"], check=True)

    server = start_server()
    try:
        # 1. Test Employee: Add Passenger & Duplicate Check
        s_emp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_emp.connect(("127.0.0.1", 9091))
        read_until(s_emp, "Select role: ")
        send_line(s_emp, "2") # Agent
        read_until(s_emp, "Username: ")
        send_line(s_emp, "employee1")
        read_until(s_emp, "Password: ")
        send_line(s_emp, "pass123")
        read_until(s_emp, "Choice: ")

        # Try adding existing passenger (customer1)
        send_line(s_emp, "1") # Add New Passenger
        read_until(s_emp, "Enter username: ")
        send_line(s_emp, "customer1")
        dup_resp = read_until(s_emp, "Choice: ")
        assert "Username already exists" in dup_resp, f"Duplicate username was not rejected: {dup_resp}"
        print("✓ Role Test 1: Employee prevented from creating duplicate passenger username")

        # Add new passenger (david / pass123)
        send_line(s_emp, "1")
        read_until(s_emp, "Enter username: ")
        send_line(s_emp, "david")
        read_until(s_emp, "Enter password: ")
        send_line(s_emp, "pass123")
        create_resp = read_until(s_emp, "Choice: ")
        assert "Passenger created successfully" in create_resp, f"Failed to create passenger: {create_resp}"
        print("✓ Role Test 2: Employee created new passenger 'david'")
        s_emp.close()

        # 2. Test New Passenger Login
        s_dav = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_dav.connect(("127.0.0.1", 9091))
        read_until(s_dav, "Select role: ")
        send_line(s_dav, "1")
        read_until(s_dav, "Username: ")
        send_line(s_dav, "david")
        read_until(s_dav, "Password: ")
        send_line(s_dav, "pass123")
        dav_menu = read_until(s_dav, "Choice: ")
        assert "Passenger Menu" in dav_menu, f"David login failed: {dav_menu}"
        print("✓ Role Test 3: New passenger 'david' logged in successfully")

        # David submits feedback
        send_line(s_dav, "6") # Add Feedback
        read_until(s_dav, "Enter your feedback: ")
        send_line(s_dav, "Great seat locking feature!")
        fb_resp = read_until(s_dav, "Choice: ")
        assert "Feedback submitted successfully" in fb_resp, f"Feedback failed: {fb_resp}"
        print("✓ Role Test 4: Passenger submitted feedback")

        # David changes password
        send_line(s_dav, "5") # Change password
        read_until(s_dav, "Enter new password: ")
        send_line(s_dav, "newpass123")
        pwd_resp = read_until(s_dav, "Choice: ")
        assert "Password updated successfully" in pwd_resp, f"Password update failed: {pwd_resp}"

        # Logout David
        send_line(s_dav, "7")
        read_until(s_dav, "Select role: ")
        send_line(s_dav, "5") # Exit
        s_dav.close()

        # 3. Test Manager: View Feedback & Toggle Account Activation
        s_mgr = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_mgr.connect(("127.0.0.1", 9091))
        read_until(s_mgr, "Select role: ")
        send_line(s_mgr, "3") # Manager
        read_until(s_mgr, "Username: ")
        send_line(s_mgr, "manager1")
        read_until(s_mgr, "Password: ")
        send_line(s_mgr, "pass123")
        read_until(s_mgr, "Choice: ")

        # View Feedback
        send_line(s_mgr, "3") # View Customer Feedback
        view_fb = read_until(s_mgr, "Choice: ")
        assert "Great seat locking feature!" in view_fb, f"Feedback missing from manager view: {view_fb}"
        print("✓ Role Test 5: Manager viewed passenger feedback")

        # Deactivate David (ID 5)
        send_line(s_mgr, "1") # Activate/Deactivate
        read_until(s_mgr, "Enter Passenger ID to toggle active status: ")
        send_line(s_mgr, "5")
        deact_resp = read_until(s_mgr, "Choice: ")
        assert "DEACTIVATED successfully" in deact_resp, f"Deactivation failed: {deact_resp}"
        print("✓ Role Test 6: Manager deactivated passenger 'david'")
        s_mgr.close()

        # 4. Verify Deactivated Passenger is Blocked from Login
        s_blocked = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_blocked.connect(("127.0.0.1", 9091))
        read_until(s_blocked, "Select role: ")
        send_line(s_blocked, "1")
        read_until(s_blocked, "Username: ")
        send_line(s_blocked, "david")
        read_until(s_blocked, "Password: ")
        send_line(s_blocked, "newpass123")
        blocked_resp = read_until(s_blocked, "Select role: ")
        assert "Account is deactivated" in blocked_resp, f"Deactivated user was not blocked: {blocked_resp}"
        s_blocked.close()
        print("✓ Role Test 7: Deactivated passenger correctly blocked from login with clear notification")

        print("\n🎉 ALL ROLE & RBAC TESTS PASSED PERFECTLY!")

    finally:
        server.terminate()
        server.wait()

if __name__ == "__main__":
    run_role_tests()
