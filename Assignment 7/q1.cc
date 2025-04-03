#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/error-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FourNodeTopologyWithErrorModel");

int main(int argc, char *argv[]) {
    // Enable logging
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    // Create 4 nodes: Node0, Node1, Node2, Node3
    NodeContainer nodes;
    nodes.Create(4);

    // Setup point-to-point links between nodes:
    // Link0: Node0 <-> Node1
    // Link1: Node1 <-> Node2 (error model applied)
    // Link2: Node2 <-> Node3
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms"));

    NetDeviceContainer dev0_1 = p2p.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer dev1_2 = p2p.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer dev2_3 = p2p.Install(nodes.Get(2), nodes.Get(3));


	Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
	em->SetAttribute("ErrorRate", DoubleValue(1e-7));
	// Explicitly set the error unit to per bit
	em->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_BIT"));
	dev1_2.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));


    // Install internet stack on all nodes
    InternetStackHelper stack;
    stack.Install(nodes);

    // Enable IP forwarding on intermediate nodes (Node1 and Node2)
    for (uint32_t i = 1; i < nodes.GetN()-1; i++) {
        Ptr<Ipv4> ipv4 = nodes.Get(i)->GetObject<Ipv4>();
        ipv4->SetAttribute("IpForward", BooleanValue(true));
    }

    // Assign IP addresses for each link
    Ipv4AddressHelper ipv4;
    // Link0: Node0 <-> Node1, network 10.1.1.0/24
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer if0_1 = ipv4.Assign(dev0_1);
    // Link1: Node1 <-> Node2, network 10.1.2.0/24
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer if1_2 = ipv4.Assign(dev1_2);
    // Link2: Node2 <-> Node3, network 10.1.3.0/24
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer if2_3 = ipv4.Assign(dev2_3);

    // Populate global routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // ----- Set up Applications -----

    // UDP Flow 1: from Node0 (client) to Node2 (server) on port 5000
    UdpServerHelper udpServer1(5000);
    ApplicationContainer serverApp1 = udpServer1.Install(nodes.Get(2));
    serverApp1.Start(Seconds(1.0));
    serverApp1.Stop(Seconds(10.0));

    UdpClientHelper udpClient1(if1_2.GetAddress(1), 5000);
    udpClient1.SetAttribute("MaxPackets", UintegerValue(1000));
    udpClient1.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    udpClient1.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp1 = udpClient1.Install(nodes.Get(0));
    clientApp1.Start(Seconds(2.0));
    clientApp1.Stop(Seconds(9.0));

    // UDP Flow 2: from Node0 (client) to Node3 (server) on port 5001
    UdpServerHelper udpServer2(5001);
    ApplicationContainer serverApp2 = udpServer2.Install(nodes.Get(3));
    serverApp2.Start(Seconds(1.0));
    serverApp2.Stop(Seconds(10.0));

    UdpClientHelper udpClient2(if2_3.GetAddress(1), 5001);
    udpClient2.SetAttribute("MaxPackets", UintegerValue(1000));
    udpClient2.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    udpClient2.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp2 = udpClient2.Install(nodes.Get(0));
    clientApp2.Start(Seconds(2.0));
    clientApp2.Stop(Seconds(9.0));

    // Enable PCAP tracing on each link
    p2p.EnablePcap("scratch/q1-node0-1", dev0_1.Get(0), true);
    p2p.EnablePcap("scratch/q1-node1-2", dev1_2.Get(0), true);
    p2p.EnablePcap("scratch/q1-node2-3", dev2_3.Get(0), true);

    // Flow Monitor for performance measurement
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Analyze FlowMonitor statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (auto iter = stats.begin(); iter != stats.end(); ++iter) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        std::cout << "Flow " << iter->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << iter->second.txPackets << "\n";
        std::cout << "  Rx Packets: " << iter->second.rxPackets << "\n";
        std::cout << "  Lost Packets: " << (iter->second.txPackets - iter->second.rxPackets) << "\n";
        std::cout << "  Packet Loss Ratio: " 
                  << ((iter->second.txPackets - iter->second.rxPackets) * 100.0 / iter->second.txPackets) << "%\n\n";
    }

    Simulator::Destroy();
    return 0;
}

