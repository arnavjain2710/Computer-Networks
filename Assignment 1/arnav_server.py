from scapy.all import *
import time

# Define the Bank Protocol
class BankProtocol(Packet):
    name = "BankProtocol"
    fields_desc = [
        StrFixedLenField("username", "", length=50),  # Ensure fixed length for username
        StrFixedLenField("password", "", length=50),  # Ensure fixed length for password
        IntField("balance", 1000),
        StrFixedLenField("transaction_type", "", length=50),  # Ensure fixed length for transaction_type
        IntField("amount", 0),
        StrFixedLenField("status", "" ,  length=50)
    ]

# Bind our custom protocol to TCP
bind_layers(TCP, BankProtocol)

# Hardcoded username and password for authentication
USERNAME = "user"
PASSWORD = "password"

# Function to authenticate the client
def authenticate(username, password):
    # print(username)
    # print(password)
    if username == USERNAME and password == PASSWORD:
        return "Authentication Success"
    else:
        return "Authentication Failed"

# Function to process deposit
def process_deposit(balance , amount):
    balance += amount
    return f"Deposit of ${amount} successful. New balance: ${balance}"

# Function to process withdrawal
def process_withdrawal(balance, amount):
    if amount <= balance:
        balance -= amount
        return f"Withdrawal of ${amount} successful. New balance: ${balance}"
    else:
        return f"Insufficient balance for withdrawal. Current balance: ${balance}"

# Function to check balance
def check_balance(balance):
    return f"Current balance: ${balance}"

# Server function to sniff and process packets continuously
def server():
    print("Server is continuously listening for incoming packets...")

    def handle_packet(packet):
        if packet.haslayer(BankProtocol):
            bank_packet = packet[BankProtocol]
            username = bank_packet.username.decode('utf-8').strip('\x00')
            password = bank_packet.password.decode('utf-8').strip('\x00')
            transaction_type = bank_packet.transaction_type.decode('utf-8').strip('\x00')
            status = bank_packet.status.decode('utf-8').strip('\x00')
            balance = bank_packet.balance
            amount = bank_packet.amount
            print(f"Received request: {bank_packet.transaction_type}")
            # print(f"Packet details: {packet.summary()}")
            # print(f"Bank Protocol Layer: {bank_packet.show()}") 

            # Process authentication request
            if transaction_type == "auth":
                status = authenticate(username, password)
                print(f"Sending response: {status}")  # Debugging: Print response status
                # print(f"dst: {packet[IP].src}")
                # print(f"sport: {packet[TCP].sport}")
                response_packet = IP(dst=packet[IP].src) / TCP(dport=packet[TCP].sport) / BankProtocol(
                    username=username,
                    password=password,
                    balance=balance,
                    transaction_type=transaction_type,
                    amount=amount,
                    status=status
                )
                send(response_packet)  # Send response to client
                # print(f"Response sent: {response_packet.summary()}")  
                return  

            # Process further transactions if authenticated
            if status == "Authentication Success":
                if transaction_type == "deposit":
                    response_message = process_deposit(balance, amount)
                    status = response_message
                    response_packet = IP(dst=packet[IP].src) / TCP(dport=packet[TCP].sport) / BankProtocol(
                        username=username,
                        password=password,
                        balance=balance,
                        transaction_type=transaction_type,
                        amount=amount,
                        status=response_message
                    )
                    send(response_packet)

                elif transaction_type == "withdrawal":
                    response_message = process_withdrawal(balance , amount)
                    status = response_message
                    response_packet = IP(dst=packet[IP].src) / TCP(dport=packet[TCP].sport) / BankProtocol(
                        username=username,
                        password=password,
                        balance=balance,
                        transaction_type=transaction_type,
                        amount=amount,
                        status=response_message
                    )
                    send(response_packet)

                elif transaction_type == "balance":
                    response_message = check_balance(balance)
                    status = response_message
                    response_packet = IP(dst=packet[IP].src) / TCP(dport=packet[TCP].sport) / BankProtocol(
                        username=username,
                        password=password,
                        balance=balance,
                        transaction_type=transaction_type,
                        amount=amount,
                        status=response_message
                    )
                    send(response_packet)

    sniff(prn=handle_packet, filter="tcp and dst port 1234", store=0, iface="lo")  # Specify the loopback interface for local traffic

# Main function to start the server
if __name__ == "__main__":
    server()
