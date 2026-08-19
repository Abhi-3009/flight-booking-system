import os
import glob

for filename in glob.glob("*.c"):
    with open(filename, 'r') as f:
        code = f.read()

    code = code.replace(
        '        int choice = atoi(buffer);',
        '        if (!is_numeric(buffer)) { send_message(sock, "Invalid choice! Please enter a number.\\n"); continue; }\n        int choice = atoi(buffer);'
    )
    
    with open(filename, 'w') as f:
        f.write(code)

