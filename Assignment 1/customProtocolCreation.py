from scapy.all import *
class StudentProtocol(Packet):
    name = "StudentProtocol"
    fields_desc = [
        StrField("name", ""),
    ]
bind_layers(TCP, StudentProtocol)
# Create two packets
packet1 = IP(dst="127.0.0.1") / TCP(dport=1234) / StudentProtocol(name="Ali")
packet2 = IP(dst="127.0.0.1") / TCP(dport=5678) / StudentProtocol(name="Bob")
# Combine the packets into a list
packets = [packet1, packet2]
# Save the packets to a pcap file
wrpcap("custom_packets.pcap", packets)