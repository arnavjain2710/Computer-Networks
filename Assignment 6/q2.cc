#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h" // Required for FlowMonitor
#include "ns3/netanim-module.h"      // Required for TraceMatrices

using namespace ns3;

int main() {
    // 1. Enable basic logging
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    // 2. Create topology
    NodeContainer lan1, lan2;
    lan1.Create(4); // LAN1: n0-n3
    lan2.Create(4); // LAN2: n4-n7

    // 3. Connect LANs via P2P (n3-n4)
    NodeContainer p2pNodes;
    p2pNodes.Add(lan1.Get(3));
    p2pNodes.Add(lan2.Get(0));

    // 4. Configure links
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // 5. Install devices
    NetDeviceContainer lan1Devs = csma.Install(lan1);
    NetDeviceContainer lan2Devs = csma.Install(lan2);
    NetDeviceContainer p2pDevs = p2p.Install(p2pNodes);

    // 6. Install internet stacks
    InternetStackHelper stack;
    stack.Install(lan1);
    stack.Install(lan2);

    // 7. Assign IPs
    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0");
    ip.Assign(lan1Devs);
    
    ip.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer lan2Ifs = ip.Assign(lan2Devs);
    
    ip.SetBase("10.1.3.0", "255.255.255.0");
    ip.Assign(p2pDevs);

    // 8. Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    lan1.Get(3)->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));
    lan2.Get(0)->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));

    // 9. Setup server on LAN2's n2 (10.1.2.3)
    UdpServerHelper server(9);
    ApplicationContainer serverApp = server.Install(lan2.Get(2));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // 10. Setup client on LAN1's n1
    UdpClientHelper client(lan2Ifs.GetAddress(2), 9);
    client.SetAttribute("MaxPackets", UintegerValue(100));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
    client.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp = client.Install(lan1.Get(1));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(9.0));

    // 11. Enable PCAP tracing
    csma.EnablePcap("lab6-lan1", lan1Devs.Get(1), true);
    csma.EnablePcap("lab6-lan2", lan2Devs.Get(2), true);
    p2p.EnablePcap("lab6-p2p", p2pDevs.Get(0), true);

    // 12. Setup FlowMonitor (for .xml output)
    FlowMonitorHelper flowMon;
    Ptr<FlowMonitor> monitor = flowMon.InstallAll();

    // 13. Setup TraceMatrices (for .tr output)
    AsciiTraceHelper ascii;
    csma.EnableAsciiAll(ascii.CreateFileStream("lab6.tr"));

    // 14. Run simulation
    Simulator::Stop(Seconds(11.0));
    Simulator::Run();

    // 15. Save FlowMonitor results
    monitor->SerializeToXmlFile("lab6-flowmon.xml", true, true);

    Simulator::Destroy();
    return 0;
}
