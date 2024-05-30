import asyncio
from bleak import BleakClient, BleakScanner
import struct
from pynput.mouse import Controller, Button
import logging

DEVICE_NAME = "Group1"
CHARACTERISTIC_UUID = (
    "0000a003-0000-1000-8000-00805f9b34fb"  # Replace with the actual UUID
)
BUTTON_UUID = "0000a001-0000-1000-8000-00805f9b34fb"  # Replace with the actual UUID

TOTAL_DATA = {
    "px": 0,
    "py": 0,
    "pz": 0,
    "gx": 0,
    "gy": 0,
    "gz": 0,
    "count": 0,
}
EXE_COUNT = 1
dt = 10 / 1000
threshold = 300
max_value = 1000
max_speed = 50
mouse = Controller()

logging.basicConfig(level=logging.INFO)


def byte_to_float(byte):
    return struct.unpack("f", byte)[0]


def get_command():
    px = TOTAL_DATA["px"] / EXE_COUNT
    py = TOTAL_DATA["py"] / EXE_COUNT
    pz = TOTAL_DATA["pz"] / EXE_COUNT
    gx = TOTAL_DATA["gx"] / EXE_COUNT
    gy = TOTAL_DATA["gy"] / EXE_COUNT
    gz = TOTAL_DATA["gz"] / EXE_COUNT
    if abs(px) < threshold:
        px = 0
    if abs(py) < threshold:
        py = 0
    px = px / max_value * max_speed
    py = py / max_value * max_speed
    return px, py


def exec_command(command):
    mouse.move(-command[1], command[0])


async def notification_handler(sender: int, data: bytearray):
    px = int.from_bytes(data[0:2], byteorder="little", signed=True)
    py = int.from_bytes(data[2:4], byteorder="little", signed=True)
    pz = int.from_bytes(data[4:6], byteorder="little", signed=True)
    gx = byte_to_float(data[8:12])
    gy = byte_to_float(data[12:16])
    gz = byte_to_float(data[16:20])

    logging.info(
        f"Notification received - px: {px}, py: {py}, pz: {pz}, gx: {gx}, gy: {gy}, gz: {gz}"
    )

    TOTAL_DATA["px"] += px
    TOTAL_DATA["py"] += py
    TOTAL_DATA["pz"] += pz
    TOTAL_DATA["gx"] += gx
    TOTAL_DATA["gy"] += gy
    TOTAL_DATA["gz"] += gz
    TOTAL_DATA["count"] += 1

    if TOTAL_DATA["count"] == EXE_COUNT:
        command = get_command()
        exec_command(command)
        TOTAL_DATA["px"] = 0
        TOTAL_DATA["py"] = 0
        TOTAL_DATA["pz"] = 0
        TOTAL_DATA["gx"] = 0
        TOTAL_DATA["gy"] = 0
        TOTAL_DATA["gz"] = 0
        TOTAL_DATA["count"] = 0


async def button_handler(sender: int, data: bytearray):
    logging.info(f"Button data: {data.hex()}")
    # mouse.click(Button.left)


async def run():
    devices = await BleakScanner.discover()
    target_device = None
    for device in devices:
        if DEVICE_NAME == device.name:
            target_device = device
            break

    if not target_device:
        logging.error("Device not found")
        return

    async with BleakClient(target_device) as client:
        logging.info("Connected to the device")

        services = await client.get_services()
        logging.info("Services:")
        for service in services:
            logging.info(service)

        if (
            CHARACTERISTIC_UUID
            in client.services.get_characteristic(CHARACTERISTIC_UUID).uuid
        ):
            char = client.services.get_characteristic(CHARACTERISTIC_UUID)
            if "read" in char.properties:
                data = await client.read_gatt_char(CHARACTERISTIC_UUID)
                logging.info(f"Initial read data: {data.hex()}")

            await client.start_notify(CHARACTERISTIC_UUID, notification_handler)

        if BUTTON_UUID in client.services.get_characteristic(BUTTON_UUID).uuid:
            char = client.services.get_characteristic(BUTTON_UUID)
            if "read" in char.properties:
                data = await client.read_gatt_char(BUTTON_UUID)
                logging.info(f"Initial read data: {data.hex()}")

            await client.start_notify(BUTTON_UUID, button_handler)

        logging.info("Waiting for notifications...")
        try:
            while True:
                await asyncio.sleep(1)
        finally:
            await client.stop_notify(CHARACTERISTIC_UUID)
            await client.stop_notify(BUTTON_UUID)


if __name__ == "__main__":
    asyncio.run(run())
