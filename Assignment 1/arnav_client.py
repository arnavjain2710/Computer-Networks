from scapy.all import *
import time

class BankProtocol(Packet):
    name = "BankProtocol"
    fields_desc = [
        StrFixedLenField("username", "", length=50),  
        StrFixedLenField("password", "", length=50),  
        IntField("balance", 1000),
        StrFixedLenField("transaction_type", "", length=50),  
        IntField("amount", 0),
        StrFixedLenField("status", "", length=50)
    ]

bind_layers(TCP, BankProtocol)

# Function to send authentication packet
def send_auth_packet(username, password , transaction_type="auth"):
    # Ensure no extra spaces or padding
    username = username[:50].ljust(50, '\x00')  # Padding to 50 chars if needed
    password = password[:50].ljust(50, '\x00')  # Padding to 50 chars if needed
    transaction_type = transaction_type[:50].ljust(50, '\x00')  # Padding to 50 chars if needed
    auth_packet = IP(dst="127.0.0.1") / TCP(dport=1234) / BankProtocol(
        username=username, password=password, transaction_type=transaction_type
    )
    # print(f"Sending auth request: {auth_packet.summary()}")
    send(auth_packet)  

# Function to send authentication packet
def send_query_packet(username, password , transaction_type , amount , balance  ):
    # Ensure no extra spaces or padding
    username = username[:50].ljust(50, '\x00')  # Padding to 50 chars if needed
    password = password[:50].ljust(50, '\x00')  # Padding to 50 chars if needed
    transaction_type = transaction_type[:50].ljust(50, '\x00')  # Padding to 50 chars if needed
    status = "Authentication Success"
    auth_packet = IP(dst="127.0.0.1") / TCP(dport=1234) / BankProtocol(
        username=username, password=password, transaction_type=transaction_type , status = status , amount = amount , balance = balance
    )
    # print(f"Sending auth request: {auth_packet.summary()}")
    send(auth_packet)  

# Function to listen for responses from the server
def listen_for_responses():
    responses = []  # List to store responses

    def handle_response(packet):
        if packet.haslayer(BankProtocol):
            bank_packet = packet[BankProtocol]
            status = bank_packet.status.decode('utf-8').strip('\x00')
            balance = bank_packet.balance
            amount = bank_packet.amount
            print(f"Received response: {status}")  # Debugging: Print response status
            responses.append((balance, amount, status))  # Store response

    # Sniff network packets
    sniff(prn=handle_response, filter="tcp", store=0, iface="lo", timeout=5)  

    # Return the first response if available, else return None
    return responses[0] if responses else None


# Main function to run the client
def client():
    while True:
        username = input("Enter username: ")
        password = input("Enter password: ")

        # Send authentication request
        send_auth_packet(username, password)

        # Listen for server's response
        response = listen_for_responses()
        balance , amount , status = response[0] , response[1] , response[2]

        if status == "Authentication Success":
            print("Login successful!")
            while True:
                print("1. Deposit")
                print("2. Withdraw")
                print("3. Check Balance")
                print("4. Exit")
                choice = input("Enter choice: ")
                if choice == "1":
                    amount = int(input("Enter amount to deposit: "))
                    send_query_packet(username, password, "deposit", amount , balance)
                    response = listen_for_responses()
                    balance , amount , status = response[0] , response[1] , response[2]
                    print(f"Deposit status: {status}")
                if choice == "2":
                    amount = int(input("Enter amount to withdraw: "))
                    send_query_packet(username, password, "withdrawal", amount , balance)
                    response = listen_for_responses()
                    balance , amount , status = response[0] , response[1] , response[2]
                    print(f"Withdrawal status: {status}")
                if choice == "3":
                    send_query_packet(username, password, "balance", amount , balance)
                    response = listen_for_responses()
                    balance , amount , status = response[0] , response[1] , response[2]
                    print(f"Balance: {balance}")
                if choice == "4":
                    print("Exiting...")
                    return
                
        else:
            print("Authentication failed. Please try again.")
            continue  # Re-login if authentication fails

if __name__ == "__main__":
    client()
