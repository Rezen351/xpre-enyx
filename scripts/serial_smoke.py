"""Integration checks over USB Serial. Requires pyserial; closes port on exit.
Does not exercise deep sleep/reboot, verify pixels/audio, or connect BLE.
"""
import argparse
import json
import time
from pathlib import Path
import serial

parser = argparse.ArgumentParser()
parser.add_argument("--port", default="COM15")
parser.add_argument("--output", default="docs/serial-test-results.json")
args = parser.parse_args()
results = []
transcript = []

def read_for(port, duration):
    end = time.monotonic() + duration
    output = bytearray()
    while time.monotonic() < end:
        output.extend(port.read(max(1, port.in_waiting)))
    text = output.decode("utf-8", errors="replace")
    transcript.append(text)
    return text

def check(port, name, data, expected, duration=0.7):
    port.write(data)
    port.flush()
    output = read_for(port, duration)
    ok = all(token in output for token in expected)
    results.append({"name": name, "passed": ok, "response": output})
    print(("PASS " if ok else "FAIL ") + name, flush=True)
    return output

try:
    with serial.Serial() as port:
        port.port = args.port
        port.baudrate = 115200
        port.timeout = 0.1
        port.dtr = False
        port.rts = False
        port.open()
        read_for(port, 1)
        check(port, "help CRLF", b"help\r\n", ["Xpre-Enyx commands", "test"])
        check(port, "status LF", b"status\n", ["Xpre-Enyx uptime=", "DHT11:", "Encoder:"])
        check(port, "unknown command", b"invalid_command\n", ["ERR unknown"])
        check(port, "overflow rejected and parser recovers", b"x"*180+b"\nstatus\n", ["ERR command too long", "Xpre-Enyx uptime="])
        check(port, "backspace and CR", b"statuX\x08s\r", ["Xpre-Enyx uptime="])
        check(port, "invalid mood", b"mood invalid\n", ["ERR invalid mood"])
        check(port, "invalid sensitivity", b"sensitivity nan\n", ["ERR sensitivity"])
        check(port, "menu entry", b"menu\n", ["Menu open"])
        check(port, "return to face", b"face\n", ["Face mode"])
        check(port, "notification", b"notify Integration test\n", ["NOTIFICATION: Integration test"])
        check(port, "sensor test starts", b"test dht\n", ["TEST START dht"])
        check(port, "console responsive during test", b"status\n", ["Xpre-Enyx uptime="])
        check(port, "test cancellation", b"test stop\n", ["TEST stopped"])
        check(port, "unknown test rejected", b"test invalid\n", ["ERR test target"])
        check(port, "full diagnostic completes", b"test\n", ["TEST START all", "TEST END all"], 15.5)
        check(port, "status after diagnostics", b"status\n", ["Xpre-Enyx uptime=", "BLE="])
        check(port, "stream on", b"stream on\n", ["Stream ON", "Xpre-Enyx uptime="], 2.5)
        check(port, "stream off", b"stream off\n", ["Stream OFF"])
except (serial.SerialException, OSError) as error:
    results.append({"name": "serial transport", "passed": False, "error": str(error)})
    print("SERIAL ERROR: " + str(error), flush=True)
finally:
    payload = {"port": args.port, "results": results, "transcript": transcript}
    Path(args.output).write_text(json.dumps(payload, indent=2), encoding="utf-8")
    passed = sum(item["passed"] for item in results)
    print(f"{passed}/{len(results)} passed; saved {args.output}", flush=True)
raise SystemExit(0 if results and all(item["passed"] for item in results) else 1)
