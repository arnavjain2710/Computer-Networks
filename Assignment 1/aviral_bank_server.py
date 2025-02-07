#!/usr/bin/env python3
from scapy.all import sniff, send, IP, UDP, wrpcap, Raw

# Configuration for the bank server
BANK_IP = "127.0.0.1"      # Bank server IP (localhost)
BANK_PORT = 12345          # Bank server listening port
captured_packets = []      # List to store captured packets

def process_packet(pkt):
    if pkt.haslayer(Raw):
        payload = pkt[Raw].load.decode('utf-8')
        client_ip = pkt[IP].src
        client_port = pkt[UDP].sport
        print(f"[Server] Received from {client_ip}:{client_port} -> {payload}")

        # Determine response based on the payload
        if payload.startswith("AUTH:"):
            response_payload = "AUTH_SUCCESS"
        elif payload == "BALANCE_INQUIRY":
            response_payload = "BALANCE: $1000"
        elif payload.startswith("WITHDRAW:"):
            response_payload = payload.replace("WITHDRAW", "WITHDRAW_SUCCESS")
        else:
            response_payload = "UNKNOWN_COMMAND"

        # Build and send the response packet (swap source/destination)
        response_pkt = IP(src=BANK_IP, dst=client_ip) / \
                       UDP(sport=BANK_PORT, dport=client_port) / \
                       Raw(load=response_payload)
        send(response_pkt, verbose=False)
        print(f"[Server] Sent to {client_ip}:{client_port} -> {response_payload}")

        # Append the received and sent packets to the list
        captured_packets.append(pkt)
        captured_packets.append(response_pkt)

print("[Server] Bank UDP Server is running and listening for packets...")
# Sniff UDP packets destined to BANK_PORT
sniff(iface="lo", filter=f"udp and dst port {BANK_PORT}", prn=process_packet)

# Write captured packets to a PCAP file
wrpcap('server_packets.pcap', captured_packets)
print("[Server] Captured packets have been saved to server_packets.pcap")