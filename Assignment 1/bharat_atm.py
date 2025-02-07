from scapy.all import wrpcap, Raw, Ether, IP, TCP
import time
import json

# Simulated bank database
bank_database = {
    "user1": {
        "password": "pass1",
        "balance": 1000,
        "transactions": []
    },
    "user2": {
        "password": "pass2",
        "balance": 2000,
        "transactions": []
    }
}

def create_packet(packet_type, username, password=None, transaction_details=None):
    """Create a packet for network capture"""
    data = dict(
        type="client_request",
        packet_type=packet_type,
        timestamp=time.time(),
        username=username,
        password=password,
        transaction_details=transaction_details
    )
    # Create a Scapy packet with the data
    return Ether()/IP(src="127.0.0.1", dst="127.0.0.1")/TCP(sport=1234, dport=5678)/Raw(load=json.dumps(data))

def create_response_packet(response_type, username, response_data):
    """Create a response packet for network capture"""
    data = dict(
        type="server_response",
        response_type=response_type,
        username=username,
        timestamp=time.time(),
        response=response_data
    )
    # Create a Scapy packet with the data
    return Ether()/IP(src="127.0.0.1", dst="127.0.0.1")/TCP(sport=5678, dport=1234)/Raw(load=json.dumps(data))

def process_transaction(username, transaction_type, amount=0):
    """Process bank transactions"""
    if username not in bank_database:
        return "User not found", False
    
    if transaction_type == "deposit":
        bank_database[username]["balance"] += amount
        bank_database[username]["transactions"].append(f"Deposit: ${amount}")
        return f"Deposit successful. New balance: ${bank_database[username]['balance']}", True
    
    elif transaction_type == "withdraw":
        if bank_database[username]["balance"] >= amount:
            bank_database[username]["balance"] -= amount
            bank_database[username]["transactions"].append(f"Withdrawal: ${amount}")
            return f"Withdrawal successful. New balance: ${bank_database[username]['balance']}", True
        return "Insufficient funds", False

def main():
    packets = []
    print("Welcome to ATM System")
    
    # Authentication
    username = input("Enter username: ")
    password = input("Enter password: ")
    
    # Create and send authentication packet
    auth_packet = create_packet("auth_request", username, password)
    packets.append(auth_packet)
    
    # Verify credentials
    if username not in bank_database or bank_database[username]["password"] != password:
        auth_response = create_response_packet("auth_response", username, "Authentication failed")
        packets.append(auth_response)
        print("Authentication failed")
        return packets
    
    auth_response = create_response_packet("auth_response", username, "Authentication successful")
    packets.append(auth_response)
    print("Authentication successful")

    while True:
        print("\n1. Check Balance")
        print("2. Withdraw")
        print("3. Deposit")
        print("4. Exit")
        
        choice = input("Enter choice (1-4): ")
        
        if choice == "1":
            # Balance check packet
            balance_packet = create_packet("balance_request", username)
            packets.append(balance_packet)
            
            balance = bank_database[username]["balance"]
            balance_response = create_response_packet("balance_response", username, f"Current balance: ${balance}")
            packets.append(balance_response)
            print(f"Current balance: ${balance}")
            
        elif choice == "2":
            amount = int(input("Enter amount to withdraw: $"))
            # Withdrawal packet
            withdraw_packet = create_packet("withdrawal_request", username, 
                                         transaction_details=f"Amount: ${amount}")
            packets.append(withdraw_packet)
            
            result, success = process_transaction(username, "withdraw", amount)
            withdraw_response = create_response_packet("withdrawal_response", username, result)
            packets.append(withdraw_response)
            print(result)
            
        elif choice == "3":
            amount = int(input("Enter amount to deposit: $"))
            # Deposit packet
            deposit_packet = create_packet("deposit_request", username, 
                                        transaction_details=f"Amount: ${amount}")
            packets.append(deposit_packet)
            
            result, success = process_transaction(username, "deposit", amount)
            deposit_response = create_response_packet("deposit_response", username, result)
            packets.append(deposit_response)
            print(result)
            
        elif choice == "4":
            # Exit packet
            exit_packet = create_packet("exit_request", username)
            packets.append(exit_packet)
            break
    
    # Save all transaction packets to pcap file
    wrpcap("atm_transactions.pcap", packets)
    print("\nSession ended. Transaction log saved.")
    return packets

if __name__ == "__main__":
    main()