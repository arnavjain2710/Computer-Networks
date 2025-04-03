#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("InterSubnetRouting");

int main(int argc, char *argv[])
{
  // Enable logging
  LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

  // Create nodes
  NodeContainer client, router, server;
  client.Create(1);
  router.Create(1);
  server.Create(1);

  // Create links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  NetDeviceContainer devClientRouter = p2p.Install(client.Get(0), router.Get(0));
  NetDeviceContainer devRouterServer = p2p.Install(router.Get(0), server.Get(0));

  // Install internet stack
  InternetStackHelper stack;
  stack.InstallAll(); // Install on all nodes at once

  // Enable IP forwarding on router ************** KEY FIX 1 **************
  router.Get(0)->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));

  // Assign IP addresses
  Ipv4AddressHelper ipv4;
  
  // Client-Router network (192.168.1.0/24)
  ipv4.SetBase("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifClientRouter = ipv4.Assign(devClientRouter);
  
  // Router-Server network (192.168.2.0/24)
  ipv4.SetBase("192.168.2.0", "255.255.255.0");
  Ipv4InterfaceContainer ifRouterServer = ipv4.Assign(devRouterServer);

  // Configure routing ************** KEY FIX 2 **************
  Ipv4StaticRoutingHelper routingHelper;
  
  // Client configuration
  Ptr<Ipv4StaticRouting> clientRouting = routingHelper.GetStaticRouting(client.Get(0)->GetObject<Ipv4>());
  // Set default route to router
  clientRouting->AddNetworkRouteTo(Ipv4Address("0.0.0.0"), Ipv4Mask("0.0.0.0"), ifClientRouter.GetAddress(1), 1);

  // Server configuration
  Ptr<Ipv4StaticRouting> serverRouting = routingHelper.GetStaticRouting(server.Get(0)->GetObject<Ipv4>());
  // Set default route to router
  serverRouting->AddNetworkRouteTo(Ipv4Address("0.0.0.0"), Ipv4Mask("0.0.0.0"), ifRouterServer.GetAddress(0), 1);

  // Applications setup
  UdpServerHelper serverHelper(6000);
  ApplicationContainer serverApp = serverHelper.Install(server.Get(0));
  serverApp.Start(Seconds(1.0));
  serverApp.Stop(Seconds(10.0));

  UdpClientHelper clientHelper(ifRouterServer.GetAddress(1), 6000);
  clientHelper.SetAttribute("MaxPackets", UintegerValue(500));
  clientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(20)));
  clientHelper.SetAttribute("PacketSize", UintegerValue(1024));
  ApplicationContainer clientApp = clientHelper.Install(client.Get(0));
  clientApp.Start(Seconds(2.0));
  clientApp.Stop(Seconds(9.0));

  // Enable PCAP tracing
  p2p.EnablePcapAll("inter-subnet");

  // Flow monitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop(Seconds(10.0));
  Simulator::Run();

  // Analyze results
  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

  for (auto &iter : stats)
  {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter.first);
    std::cout << "\nFlow " << iter.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Tx Packets: " << iter.second.txPackets << "\n";
    std::cout << "  Rx Packets: " << iter.second.rxPackets << "\n";
    std::cout << "  Lost Packets: " << iter.second.lostPackets << "\n";
    std::cout << "  Throughput: " << iter.second.rxBytes * 8.0 / 9.0 / 1000 << " Kbps\n";
  }

  Simulator::Destroy();
  return 0;
}
