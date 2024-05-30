import asyncio
from bleak import BleakClient, BleakScanner
import struct
from pynput.mouse import Controller, Button
import time

DEVICE_NAME = "Group1"
CHARACTERISTIC_UUID = (
    "0000a003-0000-1000-8000-00805f9b34fb"  # Replace with the actual UUID
)
BUTTON_UUID = "0000a001-0000-1000-8000-00805f9b34fb"  # Replace with the actual UUID

TOTAL_DATA = {
    "px": 0,
    "py": 0,
    # "pz": 0,
    # "gx": 0,
    # "gy": 0,
    # "gz": 0,
    "count": 0,
}
EXE_COUNT = 1
dt = 10 / 1000
threshold = 300
max_value = 1000
max_speed = 100
min_speed = 10
mouse = Controller()
max_x = 1920
max_y = 1080

# button
first_p_time = -1
first_r_time = -1
second_p_time = -1
second_r_time = -1
scroll = False
left_click_task = None
left_press_task = None


def byte_to_float(byte):
    return struct.unpack("f", byte)[0]


def get_command():
    px = TOTAL_DATA["px"] / (EXE_COUNT)
    py = TOTAL_DATA["py"] / (EXE_COUNT)
    # pz = TOTAL_DATA["pz"] / (EXE_COUNT)
    # gx = TOTAL_DATA["gx"] / (EXE_COUNT)
    # gy = TOTAL_DATA["gy"] / (EXE_COUNT)
    # gz = TOTAL_DATA["gz"] / (EXE_COUNT)
    # print(f"p = ({px}, {py}, {pz})\tg = ({gx}, {gy}, {gz})")
    if abs(px) < threshold:
        px = 0
    if abs(py) < threshold:
        py = 0
    px = ((px / max_value) * abs(px / max_value)) * max_speed
    py = ((py / max_value) * abs(py / max_value)) * max_speed
    return px, py


def exec_command(command):
    # mouse.move(-command[1], command[0])
    if scroll:
        mouse.scroll(command[1] / 5, command[0] / 5)
    else:
        pos = [mouse.position[0], mouse.position[1]]
        next_pos = [
            max(0, min(max_x, pos[0] - command[1])),
            max(0, min(max_y, pos[1] + command[0])),
        ]
        # mouse.position = pos
        mouse.move(next_pos[0] - pos[0], next_pos[1] - pos[1])


async def notification_handler(sender: int, data: bytearray):
    print(data)
    px = int.from_bytes(data[0:2], byteorder="little", signed=True)
    py = int.from_bytes(data[2:4], byteorder="little", signed=True)
    TOTAL_DATA["px"] += px
    TOTAL_DATA["py"] += py
    TOTAL_DATA["count"] += 1
    if TOTAL_DATA["count"] == EXE_COUNT:
        command = get_command()
        exec_command(command)
        TOTAL_DATA["px"] = 0
        TOTAL_DATA["py"] = 0
        TOTAL_DATA["count"] = 0


async def delay_click(seconds):
    # suspend for a time limit in seconds
    await asyncio.sleep(seconds)
    # execute the other coroutine
    mouse.click(Button.left)


async def delay_press(seconds):
    # suspend for a time limit in seconds
    await asyncio.sleep(seconds)
    # execute the other coroutine
    mouse.press(Button.left)


def reset_time():
    global first_p_time, first_r_time, second_p_time, second_r_time
    first_p_time = -1
    first_r_time = -1
    second_p_time = -1
    second_r_time = -1


async def button_handler(sender: int, data: bytearray):
    # print(f"Button data: {data.hex()}")
    data = int.from_bytes(data, byteorder="little", signed=True)
    status = data % 2
    # print(f"Button {count} is {'pressed' if status else 'released'}")

    # * Implement the button handler
    # if press and release quickly, click left
    # if press and release twice quickly, click right
    # if press and hold, set scroll to True
    # Specifically:
    # Scheduling a function mouse.click(Button.left) to be executed after 0.5 second delay after button released
    # If another button is pressed within 0.5 second, cancel the schedule and wait until the button released, then do mouse.click(Button.right)
    global first_p_time, first_r_time, second_p_time, second_r_time, scroll, left_click_task, left_press_task

    click_thresh = 0.3
    scroll_thresh = 0.3
    current_time = time.time()
    if status == 1:  # Button pressed
        if first_p_time == -1:  # First press
            scroll = True
            first_p_time = current_time
        elif (
            current_time - first_p_time > click_thresh
        ):  # Reset if time interval is too long
            reset_time()
            first_p_time = current_time
        else:  # Second press
            if not left_click_task.done():
                left_click_task.cancel()
                left_click_task = None
                left_press_task = asyncio.create_task(delay_press(scroll_thresh))
                second_p_time = current_time
            else:
                left_click_task = None
                first_p_time = current_time
                first_r_time = -1
        return
    else:  # Button released
        scroll = False
        if first_r_time == -1:  # First release
            if current_time - first_p_time < scroll_thresh:  # Single press candidate
                left_click_task = asyncio.create_task(delay_click(click_thresh))
                first_r_time = current_time
            else:  # Long press
                reset_time()
                return
        elif second_r_time == -1:  # Second release
            # if current_time - second_p_time > scroll_thresh: # Single press candidate
            if left_press_task.done():
                mouse.release(Button.left)
            else:
                left_press_task.cancel()
                mouse.click(Button.right)
            left_press_task = None
            reset_time()


async def run():
    target_device = None
    while not target_device:
        devices = await BleakScanner.discover()
        for device in devices:
            # print(device)
            if DEVICE_NAME == device.name:
                target_device = device
                break

        if not target_device:
            print("Device not found")

    async with BleakClient(target_device) as client:
        print("Connected to the device")

        services = await client.get_services()
        print("Services:")
        for service in services:
            print(service)

        if (
            CHARACTERISTIC_UUID
            in client.services.get_characteristic(CHARACTERISTIC_UUID).uuid
        ):
            char = client.services.get_characteristic(CHARACTERISTIC_UUID)
            if "read" in char.properties:
                data = await client.read_gatt_char(CHARACTERISTIC_UUID)
                print(f"Initial read data: {data.hex()}")

            await client.start_notify(CHARACTERISTIC_UUID, notification_handler)

        if BUTTON_UUID in client.services.get_characteristic(BUTTON_UUID).uuid:
            char = client.services.get_characteristic(BUTTON_UUID)
            if "read" in char.properties:
                data = await client.read_gatt_char(BUTTON_UUID)
                print(f"Initial read data: {data.hex()}")

            await client.start_notify(BUTTON_UUID, button_handler)

        print("Waiting for notifications...")
        try:
            while True:
                await asyncio.sleep(1)
        finally:
            await client.stop_notify(CHARACTERISTIC_UUID)
            await client.stop_notify(BUTTON_UUID)


if __name__ == "__main__":
    asyncio.run(run())
