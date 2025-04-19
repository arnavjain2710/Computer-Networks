#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeNodeTopology");

int main(int argc, char *argv[]) {
    // Logging
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    // Create nodes
    NodeContainer nodes;
    nodes.Create(3);

    // Setup point-to-point links with variable latency
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("1ms")); 

    NetDeviceContainer dev0_1 = p2p.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer dev1_2 = p2p.Install(nodes.Get(1), nodes.Get(2));

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Enable IP forwarding on Node1 
    Ptr<Ipv4> ipv4Node1 = nodes.Get(1)->GetObject<Ipv4>();
    ipv4Node1->SetAttribute("IpForward", BooleanValue(true));

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    ipv4.Assign(dev0_1);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(dev1_2);

    // Add global routing 
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Server on Node2 (Port 5000)
    UdpServerHelper server(5000);
    ApplicationContainer serverApp = server.Install(nodes.Get(2));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // Client on Node0 (to Server Port 5000)
    UdpClientHelper client(interfaces.GetAddress(1), 5000);
    client.SetAttribute("MaxPackets", UintegerValue(1000));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    client.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(9.0));

    // Add second server (Port 5001) and client
    UdpServerHelper server2(5001);
    ApplicationContainer serverApp2 = server2.Install(nodes.Get(2));
    serverApp2.Start(Seconds(1.0));
    serverApp2.Stop(Seconds(10.0));

    UdpClientHelper client2(interfaces.GetAddress(1), 5001);
    client2.SetAttribute("MaxPackets", UintegerValue(1000));
    client2.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    client2.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp2 = client2.Install(nodes.Get(0));
    clientApp2.Start(Seconds(2.0));
    clientApp2.Stop(Seconds(9.0));

    // Enable PCAP tracing in the "scratch" directory
    p2p.EnablePcap("scratch/exp1-node0", dev0_1.Get(0), true);  // Node0 -> Node1
    p2p.EnablePcap("scratch/exp1-node1", dev1_2.Get(0), true);  // Node1 -> Node2

    // Flow Monitor for throughput measurement
    FlowMonitorHelper flowMon;
    Ptr<FlowMonitor> monitor = flowMon.InstallAll();

    // Set explicit simulation stop time
    Simulator::Stop(Seconds(10.0));  // 🔑 Fixes hanging issue
    
    Simulator::Run();
    // Calculate throughput here using FlowMonitor data
    Simulator::Destroy();
    return 0;
}
