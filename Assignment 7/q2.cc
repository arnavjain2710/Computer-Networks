#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ConcurrentFlows");

int main(int argc, char *argv[]) {
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
	LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_INFO);
	
    // Create nodes
    NodeContainer nodes;
    nodes.Create(3);

    // Setup links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer dev0_1 = p2p.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer dev1_2 = p2p.Install(nodes.Get(1), nodes.Get(2));

    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IPs
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    ipv4.Assign(dev0_1);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer iface1_2 = ipv4.Assign(dev1_2);

    nodes.Get(1)->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // UDP Server (Node2, port 5000)
    UdpServerHelper udpServer(5000);
    ApplicationContainer udpServerApp = udpServer.Install(nodes.Get(1));
    udpServerApp.Start(Seconds(1.0));
    udpServerApp.Stop(Seconds(10.0));

    // UDP Client (Node0)
    UdpClientHelper udpClient(iface1_2.GetAddress(1), 5000);
    udpClient.SetAttribute("MaxPackets", UintegerValue(1000));
    udpClient.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    udpClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer udpClientApp = udpClient.Install(nodes.Get(0));
    udpClientApp.Start(Seconds(2.0));
    udpClientApp.Stop(Seconds(9.0));

    // TCP Server (Node2, port 5001)
    PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
    ApplicationContainer tcpServerApp = tcpSink.Install(nodes.Get(2));
    tcpServerApp.Start(Seconds(1.0));
    tcpServerApp.Stop(Seconds(10.0));

    // TCP Client (Node1)
    BulkSendHelper tcpClient("ns3::TcpSocketFactory", InetSocketAddress(iface1_2.GetAddress(1), 5001));
    tcpClient.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer tcpClientApp = tcpClient.Install(nodes.Get(0));
    tcpClientApp.Start(Seconds(2.0));
    tcpClientApp.Stop(Seconds(9.0));

    // Enable PCAP
    p2p.EnablePcapAll("q2");

    // Flow Monitor
    FlowMonitorHelper flowMon;
    Ptr<FlowMonitor> monitor = flowMon.InstallAll();

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // Output metrics
    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (auto &flow : stats) {
        std::cout << "Flow " << flow.first << " (TCP/UDP): Throughput=" 
                  << flow.second.rxBytes * 8.0 / 9.0 / 1e6 << " Mbps, Loss=" 
                  << flow.second.lostPackets << "\n";
    }

    Simulator::Destroy();
    return 0;
}
