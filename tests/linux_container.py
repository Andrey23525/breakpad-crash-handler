"""Build and exercise the public client API in a disposable Linux container."""
import pathlib
import signal
import struct
import subprocess
import time

build = pathlib.Path("/tmp/breakpad-build")
subprocess.run(["cmake", "-S", ".", "-B", str(build), "-G", "Ninja",
                "-DCMAKE_BUILD_TYPE=Debug"], check=True)
subprocess.run(["cmake", "--build", str(build), "-j", "2"], check=True)
subprocess.run(["ctest", "--test-dir", str(build), "--output-on-failure"], check=True)
dumps = build / "crash dumps"
client = subprocess.Popen([str(build / "example_client"), str(build / "crash_handler"),
                           str(dumps)], start_new_session=True)
try:
    code = client.wait(timeout=30)
    assert code == -signal.SIGSEGV, f"Expected SIGSEGV, got {code}"
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline and not list(dumps.glob("*.dmp")):
        time.sleep(0.1)
    files = list(dumps.glob("*.dmp"))
    assert files, "No minidump produced"
    for dump in files:
        data = dump.read_bytes()
        assert data[:4] == b"MDMP", "Invalid minidump signature"
        count, directory = struct.unpack_from("<II", data, 8)
        streams = {}
        for index in range(count):
            kind, size, offset = struct.unpack_from("<III", data, directory + index * 12)
            assert offset + size <= len(data), "Truncated stream"
            streams[kind] = (size, offset)
        assert 6 in streams, "Missing exception stream"
        assert struct.unpack_from("<I", data, streams[6][1] + 8)[0] == signal.SIGSEGV
        assert 3 in streams, "Missing thread stream"
        assert struct.unpack_from("<I", data, streams[3][1])[0] > 0
        print(f"Verified minidump: {dump.name}, {dump.stat().st_size} bytes", flush=True)
finally:
    import os
    try:
        os.killpg(client.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
