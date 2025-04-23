#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CsmaTopology");

int main() {
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(4);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));

    NetDeviceContainer lan1 = csma.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(lan1);

    UdpEchoServerHelper server(5000);
    ApplicationContainer serverApp = server.Install(nodes.Get(3));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    UdpEchoClientHelper client(interfaces.GetAddress(3), 5000);
    client.SetAttribute("MaxPackets", UintegerValue(1000));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    client.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(10.0));

    csma.EnablePcapAll("scratch/q2");

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (const auto& flow : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        std::cout << "Flow ID: " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << flow.second.txPackets << "\n";
        std::cout << "  Rx Packets: " << flow.second.rxPackets << "\n";
        std::cout << "  Lost Packets: " << flow.second.lostPackets << "\n";
        std::cout << "  Delay Sum: " << flow.second.delaySum.GetSeconds() << " s\n";
        std::cout << "  Throughput: " 
                  << (flow.second.rxBytes * 8.0 / (flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds())) / 1e6
                  << " Mbps\n\n";
    }

    Simulator::Destroy();
}
