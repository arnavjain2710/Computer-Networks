#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpBulkSendExample");

int main() {
    // Enable logging
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    // Set MSS (TCP SegmentSize)
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1400));

    // Create 2 nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Setup point-to-point link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // PacketSink on Node 1 (Receiver)
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), 5001));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(10.0));

    // BulkSend on Node 0 (Sender)
    BulkSendHelper source("ns3::TcpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(1), 5001));
    source.SetAttribute("MaxBytes", UintegerValue(0)); // Unlimited
    source.SetAttribute("SendSize", UintegerValue(1400)); // Match MSS if desired

    ApplicationContainer sourceApp = source.Install(nodes.Get(0));
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(10.0));

    // Enable pcap tracing
    p2p.EnablePcapAll("scratch/q1");

    // FlowMonitor setup
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // FlowMonitor stats
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (auto const& flow : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        std::cout << "Flow " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Bytes: " << flow.second.txBytes << "\n";
        std::cout << "  Rx Bytes: " << flow.second.rxBytes << "\n";
        std::cout << "  Throughput: "
                  << (flow.second.rxBytes * 8.0 / (flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds()) / 1e6)
                  << " Mbps\n";
        std::cout << "  Lost Packets: " << flow.second.lostPackets << "\n\n";
    }

    Simulator::Destroy();
    return 0;
}
