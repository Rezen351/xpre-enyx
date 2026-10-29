"""Optional BLE integration check. Install bleak into .pio/test-tools first."""
import asyncio
import json
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / ".pio" / "test-tools"))
from bleak import BleakClient, BleakScanner

SERVICE = "a6e40001-b5a3-f393-e0a9-e50e24dcca9e"
RX = "a6e40002-b5a3-f393-e0a9-e50e24dcca9e"
TX = "a6e40003-b5a3-f393-e0a9-e50e24dcca9e"
results=[]
def record(name, passed, detail=""):
    results.append({"name":name,"passed":passed,"detail":detail})
    print(("PASS " if passed else "FAIL ")+name+" "+detail,flush=True)

def matches(device, advert):
    return (device.name == "Xpre-Enyx" or advert.local_name == "Xpre-Enyx") and SERVICE in advert.service_uuids

async def main():
    device=await BleakScanner.find_device_by_filter(matches,timeout=12)
    record("advertising",device is not None)
    if device is None:return
    async with BleakClient(device,timeout=15) as client:
        record("connect",client.is_connected)
        record("GATT service",client.services.get_service(SERVICE) is not None)
        received=asyncio.Event()
        messages=[]
        def on_notify(characteristic,data):
            messages.append(bytes(data).decode(errors="replace"));received.set()
        await client.start_notify(TX,on_notify)
        await client.write_gatt_char(RX,b"BLE integration test",response=True)
        await asyncio.wait_for(received.wait(),5)
        record("write + notification acknowledgement","notification received" in messages,repr(messages))
        # Rejected oversized input must leave the last valid acknowledgement intact.
        await client.write_gatt_char(RX,b"x"*97,response=True)
        await asyncio.sleep(0.5)
        record("oversized message ignored",len(messages)==1)
    await asyncio.sleep(1)
    device=await BleakScanner.find_device_by_filter(matches,timeout=12)
    record("advertising after disconnect",device is not None)

try:
    asyncio.run(main())
except Exception as error:
    record("BLE transport",False,str(error))
finally:
    Path("docs/ble-test-results.json").write_text(json.dumps(results,indent=2),encoding="utf-8")
raise SystemExit(0 if results and all(r["passed"] for r in results) else 1)
