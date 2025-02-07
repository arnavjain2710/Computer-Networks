from scapy.all import *

class BankProtocol(Packet):
    name = "BankProtocol"
    fields_desc = [
        StrField("transaction_type", ""),
        StrField("username", ""),
        StrField("amount", "0"),
        StrField("response", ""),
        StrField("balance", "0")
    ]

bind_layers(TCP, BankProtocol)

balance = 5000
user1 = "user1"
password1 = "any_password"
login=0

def create_auth_packet(username, password):
    global balance,user1,password1,login
    if (user1!=username) or (password1!=password):
        return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
            transaction_type="auth",
            username=username,
            response="Login Denied",
            balance=""
        )
    login=1
    return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
        transaction_type="auth",
        username=username,
        response="Successful Login",
        balance=str(balance)
    )

def create_balance_inquiry_packet(username):
    global balance,login
    if login==0 :
        return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
            transaction_type="balance_inquiry",
            username=username,
            amount="",
            response="Not login",
            balance=""
        )
    return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
        transaction_type="balance_inquiry",
        username=username,
        response="Balance: $" + str(balance),
        balance=str(balance)
    )

def create_withdrawal_packet(username, amount):
    global balance,login
    if login==0 :
        return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
            transaction_type="withdrawal",
            username=username,
            amount=str(amount),
            response="Not login",
            balance=""
        )
    if amount > balance:
        return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
            transaction_type="withdrawal",
            username=username,
            amount=str(amount),
            response="Insufficient funds",
            balance=str(balance)
        )
    balance -= amount
    return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
        transaction_type="withdrawal",
        username=username,
        amount=str(amount),
        response="Withdrawal Successful",
        balance=str(balance)
    )

def create_deposit_packet(username, amount):
    global balance,login
    if login==0 :
        return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
            transaction_type="deposit",
            username=username,
            amount=str(amount),
            response="Not login",
            balance=""
        )
    balance += amount
    return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
        transaction_type="deposit",
        username=username,
        amount=str(amount),
        response="Deposit Successful",
        balance=str(balance)
    )

def logout(username):
    global login
    login=0
    return IP(dst="127.0.0.1") / TCP(dport=12345) / BankProtocol(
        transaction_type="auth",
        username=username,
        response="Successful Logout",
        balance=str(balance)
    )

auth_packet1 = create_auth_packet("user1", "123")
balance_packet1 = create_balance_inquiry_packet("user1")
auth_packet2 = create_auth_packet("user1", "any_password")
balance_packet2 = create_balance_inquiry_packet("user1")
withdraw_packet = create_withdrawal_packet("user1", 100)
deposit_packet = create_deposit_packet("user1", 200)
logout_packet = logout("user1")

packets = [auth_packet1 , balance_packet1 , auth_packet2 , balance_packet2, withdraw_packet, deposit_packet ,logout_packet]

wrpcap("bank_transactions.pcap", packets)

print("Packets saved to bank_transactions.pcap")