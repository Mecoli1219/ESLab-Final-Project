# ble_scan_connect.py:
from bluepy.btle import Peripheral, UUID
from bluepy.btle import Scanner, DefaultDelegate


class NotificationDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):
        print("Received notification: Handle={}, Data={}".format(cHandle, data.hex()))


class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        # if isNewDev:
        #     print("Discovered device", dev.addr)
        # elif isNewData:
        #     print("Received new data from", dev.addr)
        pass


print("waiting for device...")
scanner = Scanner().withDelegate(ScanDelegate())
devices = scanner.scan(10.0)
dev = None
for deve in devices:
    if deve.addr == "f2:59:14:4a:02:01":
        print("Connecting...")
        dev = Peripheral(deve.addr, "random")
print("Services...")
for svc in dev.services:
    print(str(svc))
#
try:
    ch = dev.getCharacteristics(uuid=UUID(0xA003))[0]
    data = None
    if ch.supportsRead():
        while True:
            cur_data = ch.read()
            if data != cur_data:
                data = cur_data
                print(data)
    dev.setDelegate(NotificationDelegate())

    while True:
        if dev.waitForNotifications(1.0):
            continue
#
finally:
    dev.disconnect()
