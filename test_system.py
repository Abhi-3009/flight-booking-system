import socket
import time
import subprocess
import os

def start_server():
    server_proc = subprocess.Popen(["./server"], cwd=os.getcwd(), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    # Poll until socket is connectable
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

def run_tests():
    print("🚀 Starting Flight Booking System Automated Test Suite...")
    subprocess.run(["make", "setup"], check=True)
    subprocess.run(["make", "rebuild"], check=True)

    server = start_server()
    try:
        # 1. Connect Client 1 (customer1)
        s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s1.connect(("127.0.0.1", 9091))
        
        prompt = read_until(s1, "Select role: ")
        assert "Airline Booking System" in prompt, f"Unexpected banner: {prompt}"
        
        # Login customer1
        send_line(s1, "1") # Passenger
        read_until(s1, "Username: ")
        send_line(s1, "customer1")
        read_until(s1, "Password: ")
        send_line(s1, "pass123")
        
        menu = read_until(s1, "Choice: ")
        assert "Passenger Menu" in menu, f"Login failed: {menu}"
        print("✓ Test 1: Customer 1 Login successful")

        # 2. Duplicate Login Prevention Test
        s_dup = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_dup.connect(("127.0.0.1", 9091))
        read_until(s_dup, "Select role: ")
        send_line(s_dup, "1")
        read_until(s_dup, "Username: ")
        send_line(s_dup, "customer1")
        read_until(s_dup, "Password: ")
        send_line(s_dup, "pass123")
        dup_resp = read_until(s_dup, "Select role: ")
        assert "already logged in" in dup_resp, f"Expected duplicate login rejection: {dup_resp}"
        s_dup.close()
        print("✓ Test 2: Duplicate login prevented successfully")

        # 3. Connect Client 2 (customer2)
        s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s2.connect(("127.0.0.1", 9091))
        read_until(s2, "Select role: ")
        send_line(s2, "1")
        read_until(s2, "Username: ")
        send_line(s2, "customer2")
        read_until(s2, "Password: ")
        send_line(s2, "pass123")
        read_until(s2, "Choice: ")
        print("✓ Test 3: Customer 2 Login successful")

        # 4. Client 1 views seat map and locks Seat 1A
        send_line(s1, "2") # Book Ticket
        read_until(s1, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s1, "1")
        seat_map_s1 = read_until(s1, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        assert "FLIGHT 1 SEAT MAP" in seat_map_s1, f"Seat map failed: {seat_map_s1}"
        assert "[1A:FREE]" in seat_map_s1, f"1A should be FREE: {seat_map_s1}"

        # Client 1 locks 1A
        send_line(s1, "1A")
        hold_prompt = read_until(s1, "Confirm booking? (Y/N): ")
        assert "Seat 1A reserved for you" in hold_prompt, f"Failed to hold 1A: {hold_prompt}"
        print("✓ Test 4: Customer 1 successfully locked Seat 1A (Hold Active)")

        # 5. Client 2 tries to view seat map and lock 1A (should show HELD and reject)
        send_line(s2, "2") # Book ticket
        read_until(s2, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s2, "1")
        seat_map_s2 = read_until(s2, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        assert "[1A:HELD]" in seat_map_s2, f"1A should show HELD to customer 2: {seat_map_s2}"

        # Client 2 tries to reserve 1A
        send_line(s2, "1A")
        reject_msg = read_until(s2, "Choice: ")
        assert "currently locked by another passenger" in reject_msg, f"Expected lock rejection: {reject_msg}"
        print("✓ Test 5: Customer 2 correctly blocked from taking locked Seat 1A")

        # 6. Client 1 confirms booking (Y)
        send_line(s1, "Y")
        confirm_resp = read_until(s1, "Choice: ")
        assert "Ticket booked successfully" in confirm_resp, f"Booking confirm failed: {confirm_resp}"
        print("✓ Test 6: Customer 1 confirmed booking for Seat 1A")

        # 7. Verify seat is now TAKEN in database and seat map
        send_line(s2, "2")
        read_until(s2, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s2, "1")
        seat_map_s2_after = read_until(s2, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        assert "[1A:TAKEN]" in seat_map_s2_after, f"1A should now be TAKEN: {seat_map_s2_after}"
        send_line(s2, "0") # Exit booking
        read_until(s2, "Choice: ")
        print("✓ Test 7: Seat 1A shows TAKEN to all users")

        # 8. Test Hold Release on Cancellation (Customer 2 locks 1B, then cancels with N)
        send_line(s2, "2")
        read_until(s2, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s2, "1")
        read_until(s2, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        send_line(s2, "1B")
        read_until(s2, "Confirm booking? (Y/N): ")
        send_line(s2, "N") # Cancel
        cancel_resp = read_until(s2, "Choice: ")
        assert "Reservation cancelled. Seat released." in cancel_resp, f"Cancel failed: {cancel_resp}"

        # Verify 1B is FREE again
        send_line(s2, "2")
        read_until(s2, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s2, "1")
        seat_map_1b = read_until(s2, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        assert "[1B:FREE]" in seat_map_1b, f"1B should be FREE: {seat_map_1b}"
        send_line(s2, "0")
        read_until(s2, "Choice: ")
        print("✓ Test 8: Seat reservation cancellation correctly released hold back to FREE")

        # 9. Test Disconnect Hold Cleanup (Customer 2 locks 1C, then forcibly disconnects)
        send_line(s2, "2")
        read_until(s2, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s2, "1")
        read_until(s2, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        send_line(s2, "1C")
        read_until(s2, "Confirm booking? (Y/N): ")
        s2.close() # Abrupt disconnect
        time.sleep(0.5)

        # Client 1 checks seat map for 1C (should be immediately released)
        send_line(s1, "2")
        read_until(s1, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s1, "1")
        seat_map_dc = read_until(s1, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        assert "[1C:FREE]" in seat_map_dc, f"1C should be FREE after disconnect: {seat_map_dc}"
        send_line(s1, "0")
        read_until(s1, "Choice: ")
        print("✓ Test 9: Abrupt client disconnect immediately freed active seat hold")

        # 10. Test Ticket Cancellation & Seat Refund
        send_line(s1, "4") # View My Bookings
        bookings_resp = read_until(s1, "Choice: ")
        assert "1A" in bookings_resp, f"Booking 1A not found: {bookings_resp}"
        
        # Cancel booking ID 1
        send_line(s1, "3") # Cancel ticket
        read_until(s1, "Enter Booking ID to cancel: ")
        send_line(s1, "1")
        cancel_ticket_resp = read_until(s1, "Choice: ")
        assert "Ticket cancelled successfully" in cancel_ticket_resp, f"Ticket cancel failed: {cancel_ticket_resp}"

        # Verify Seat 1A is back to FREE in seat map
        send_line(s1, "2")
        read_until(s1, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s1, "1")
        seat_map_refunded = read_until(s1, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        assert "[1A:FREE]" in seat_map_refunded, f"1A should be FREE after ticket cancellation: {seat_map_refunded}"
        send_line(s1, "0")
        read_until(s1, "Choice: ")
        print("✓ Test 10: Ticket cancellation restored seat back to FREE and updated flights database")

        # 11. Test Admin Adding Flight with Auto Seat Generation
        s_admin = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_admin.connect(("127.0.0.1", 9091))
        read_until(s_admin, "Select role: ")
        send_line(s_admin, "4") # Admin
        read_until(s_admin, "Username: ")
        send_line(s_admin, "admin1")
        read_until(s_admin, "Password: ")
        send_line(s_admin, "pass123")
        read_until(s_admin, "Choice: ")

        send_line(s_admin, "1") # Add flight
        read_until(s_admin, "Enter Flight Origin: ")
        send_line(s_admin, "Kolkata")
        read_until(s_admin, "Enter Flight Destination: ")
        send_line(s_admin, "Chennai")
        read_until(s_admin, "Enter Total Seats")
        send_line(s_admin, "8")
        admin_resp = read_until(s_admin, "Choice: ")
        assert "Flight 3" in admin_resp, f"Add flight failed: {admin_resp}"
        s_admin.close()
        print("✓ Test 11: Admin added Flight 3 with automatic seat matrix creation")

        # Verify Flight 3 seat map from passenger
        send_line(s1, "2")
        read_until(s1, "Enter Flight ID to book (or 0 to cancel): ")
        send_line(s1, "3")
        seat_map_f3 = read_until(s1, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ")
        assert "FLIGHT 3 SEAT MAP" in seat_map_f3 and "[2D:FREE]" in seat_map_f3, f"Flight 3 seats failed: {seat_map_f3}"
        send_line(s1, "0")
        read_until(s1, "Choice: ")
        print("✓ Test 12: New Flight 3 seat map rendered flawlessly for passenger")

        # Clean logout
        send_line(s1, "7") # Logout
        read_until(s1, "Select role: ")
        send_line(s1, "5") # Exit
        s1.close()

        print("\n🎉 ALL 12 TESTS PASSED PERFECTLY!")

    finally:
        server.terminate()
        server.wait()

if __name__ == "__main__":
    run_tests()
