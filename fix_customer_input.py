import re

with open('customer.c', 'r') as f:
    code = f.read()

code = code.replace(
    '    int target_fid = atoi(buffer);',
    '    if (!is_numeric(buffer)) { send_message(sock, "Invalid Flight ID! Must be a number.\\n"); return; }\n    int target_fid = atoi(buffer);'
)

code = code.replace(
    '    int seats_to_book = atoi(buffer);',
    '    if (!is_numeric(buffer)) { send_message(sock, "Invalid seats! Must be a number.\\n"); return; }\n    int seats_to_book = atoi(buffer);'
)

code = code.replace(
    '    int target_bid = atoi(buffer);',
    '    if (!is_numeric(buffer)) { send_message(sock, "Invalid Booking ID! Must be a number.\\n"); return; }\n    int target_bid = atoi(buffer);'
)

with open('customer.c', 'w') as f:
    f.write(code)

