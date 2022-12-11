from csv import DictReader

def loadContacts():
    with open("contacts.csv", 'r') as f:
        contacts = list(DictReader(f))
    with open("patient.csv", 'r') as f:
        patient = list(DictReader(f))[0]
    return contacts, patient

def f_to_email(contact):
    to_emails = []
    if contact["email"] is not None:
        to_emails.append(contact["email"])
    if contact["phone"] is not None:
        carrier = contact["carrier"].lower()
        if carrier == "at&t":
            to_emails.append(contact["phone"]+"@txt.att.net")
        elif carrier == "verizon":
            to_emails.append(contact["phone"]+"@vtext.com")
        elif carrier == "virgin mobile":
            to_emails.append(contact["phone"]+"@vmobl.com")
        elif carrier == "sprint":
            to_emails.append(contact["phone"]+"@messaging.sprintpcs.com")
        elif carrier == "t-mobile":
            to_emails.append(contact["phone"]+"@tmomail.net")
        return to_emails
        

def printCHARS(client):
    for service in client.services:
        for characteristic in service.characteristics:
            print(characteristic) 

