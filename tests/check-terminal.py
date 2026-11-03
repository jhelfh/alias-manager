import os
import pty
import select
import subprocess
import sys
import time

binary = sys.argv[1]
def run(payload, expected_status, expected_text=None):
    master, slave = pty.openpty()
    process = subprocess.Popen([binary], stdin=slave, stdout=slave, stderr=slave)
    os.close(slave)
    output = bytearray()
    until = time.monotonic() + 1.2
    while time.monotonic() < until:
        ready, _, _ = select.select([master], [], [], 0.1)
        if ready:
            output.extend(os.read(master, 65536))
    assert b'\x1b[?25l' in output, output
    assert output.count(b'\x1b[?25h') >= 2, output
    os.write(master, payload)
    process.wait(timeout=5)
    while select.select([master], [], [], 0.1)[0]:
        try:
            data = os.read(master, 65536)
        except OSError:
            break
        if not data:
            break
        output.extend(data)
    os.close(master)
    assert process.returncode == expected_status, (process.returncode, output)
    if expected_text is not None:
        assert expected_text in output, output

run('hello-привет\n'.encode(), 0, 'hello-привет'.encode())
run(b'ab\x7fc\n', 0, b'ac')
run(b'\x04', 2)
print('Terminal input/blink/UTF-8/backspace/EOF checks passed.')
