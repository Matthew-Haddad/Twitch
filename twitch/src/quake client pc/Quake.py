import time
import asyncio
from bleak import BleakClient, BleakScanner
from QuakeMail import Emailer
import QuakeUtils

DEVICE_NAME = "quake"
ALERT_CHAR_UUID = "bb6f0184-7941-11ed-a1eb-0242ac120002"
VERIFY_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

async def main(address):
    try:
        emailer = Emailer(to_emails=["quakemessenger@gmail.com"])
        emailer.send_mail()
        contacts, patient = QuakeUtils.loadContacts()
    except:
        print("Emailer initialization failed.")
        exit()

    while True:
        async with BleakClient(address) as client:
            print(f"Connected: {client.is_connected}")
            while client.is_connected:
                value = bytes(await client.read_gatt_char(ALERT_CHAR_UUID))
                print(f"characteristic read: {value}")
                if (value != b''):
                    await client.write_gatt_char(VERIFY_CHAR_UUID, value)
                    sendWarn(emailer, contacts, patient, float(value))
                time.sleep(2)

def sendWarn(emailer, contacts, patient, confidence):
    print("Warning recieved, notifying contacts...")
    emailer.change_template("warn.txt")
    for contact in contacts:
        to_emails = QuakeUtils.f_to_email(contact)
        context = contact|patient|{"confidence":confidence}
        emailer.change_context(context, to_emails)
        emailer.send_mail()
    print("contacts warned successfully.")

async def find(name):
        print("searching...")
        device = await BleakScanner.find_device_by_filter(
            lambda d, ad: d.name and d.name.lower() == name.lower()
        )
        if device == None:
            return None
        return device.address

if __name__ == "__main__":
    address = asyncio.run(find(DEVICE_NAME))
    asyncio.run(main(address))