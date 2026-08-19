import re

with open('admin.c', 'r') as f:
    code = f.read()

code = code.replace(
    '    strcpy(origin, buf);',
    '    if (!is_alpha(buf)) { send_message(sock, "Invalid origin! Must be letters only.\\n"); return; }\n    strcpy(origin, buf);'
)

code = code.replace(
    '    strcpy(destination, buf);',
    '    if (!is_alpha(buf)) { send_message(sock, "Invalid destination! Must be letters only.\\n"); return; }\n    strcpy(destination, buf);'
)

code = code.replace(
    '    int total_seats = atoi(buf);',
    '    if (!is_numeric(buf)) { send_message(sock, "Invalid seats! Must be a number.\\n"); return; }\n    int total_seats = atoi(buf);'
)

with open('admin.c', 'w') as f:
    f.write(code)

