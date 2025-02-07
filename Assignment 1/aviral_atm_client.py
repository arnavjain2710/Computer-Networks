#!/usr/bin/env python3
from scapy.all import sr1, IP, UDP, wrpcap, Raw

# Configuration for the ATM client
BANK_IP = "127.0.0.1"      # Bank server IP (localhost)
BANK_PORT = 12345          # Bank server port
ATM_PORT  = 55555          # Fixed source port for the ATM client
captured_packets = []      # List to store captured packets

def send_request(request_payload):
    pkt = IP(dst=BANK_IP) / UDP(sport=ATM_PORT, dport=BANK_PORT) / Raw(load=request_payload)
    print(f"[ATM] Sending: {request_payload}")
    # Send the packet and wait for a reply
    response = sr1(pkt, timeout=2, verbose=False)
    if response and response.haslayer(Raw):
        resp_payload = response[Raw].load.decode('utf-8')
        print(f"[ATM] Received: {resp_payload}\n")
    else:
        print("[ATM] No response received.\n")

    # Append the sent and received packets to the list
    captured_packets.append(pkt)
    if response:
        captured_packets.append(response)

# 1. Authentication Request
send_request("AUTH: username=atm_user; password=secret")

# 2. Balance Inquiry
send_request("BALANCE_INQUIRY")

# 3. Withdrawal Request
send_request("WITHDRAW: amount=200")

# Write captured packets to a PCAP file
wrpcap('client_packets.pcap', captured_packets)
print("[ATM] Captured packets have been saved to client_packets.pcap")