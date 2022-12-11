import os
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

alias = "Quake"
username = os.environ.get('quake_username')
password = os.environ.get('quake_password')

# Locate message templates
FILE_PATH = os.path.abspath(__file__)
BASE_DIR = os.path.dirname(FILE_PATH)
TEMPLATE_DIR = os.path.join(BASE_DIR, "templates")

"""
    Sets up the email service
"""
class Emailer():
    def __init__(self, subject="", template_name="setup.txt", context={}, to_emails=[]):
        assert isinstance(to_emails, list)
        self.to_emails = to_emails
        self.subject = subject
        self.template_name = template_name
        self.context = context

    def change_context(self, context, to_emails):
        self.context = context
        self.to_emails = to_emails
        
    def change_template(self, template_name="setup.txt"):
        self.template_name = template_name

    def send_mail(self):
        msg = self.format_msg()
        did_send = False
        with smtplib.SMTP(host='smtp.gmail.com', port=587) as server:
            server.ehlo()
            server.starttls()
            server.login(username, password)
            try:
                server.sendmail(alias, self.to_emails, msg)
                did_send = True
            except:
                did_send = False
            server.quit()
        print("message sent: " + str(did_send))
        return did_send

    """
        helper for send_mail(self)
    """    
    def format_msg(self):
        msg = MIMEMultipart('alternative')
        msg['From'] = alias
        msg['To'] = ", ".join(self.to_emails)
        msg['Subject'] = self.subject
        if self.template_name != None:
            tmpl_str = Message(template_name = self.template_name, context = self.context)
        txt_part = MIMEText(tmpl_str.render(), 'plain')
        msg.attach(txt_part)
        return msg.as_string()

"""
    Produces messages using prewriten templates. Will not be called by user.
"""
class Message:
    template_name=""
    context=None # Dictionary for template rendering
    def __init__(self, template_name="", context=None, *args, **kwargs):
        self.template_name = template_name
        self.context = context

    def get_template(self):
        template_path = os.path.join(TEMPLATE_DIR, self.template_name)
        if not os.path.exists(template_path):
            raise Exception("path does not exist")
        with open(template_path, 'r') as f:
            template_string = f.read()
        return template_string

    def render(self, context=None):
        render_ctx=context
        if self.context != None:
            render_ctx = self.context
        else:
            if not isinstance(render_ctx, dict):
              render_ctx = {}
        template_string = self.get_template()
        return template_string.format(**render_ctx)

if __name__ == "__main__":
    e = Emailer(to_emails=["quakemessenger@gmail.com"])
    e.send_mail()
