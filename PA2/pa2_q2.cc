#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood-plus.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-static-routing-helper.h"

NS_LOG_COMPONENT_DEFINE ("pa3_q2");

using namespace ns3;

Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0;                     /* The value of the last total received bytes */

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "100Mbps";                  /* Application layer datarate. */
  std::string tcpVariant = "TcpNewReno";             /* TCP variant type. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
  double simulationTime = 10;                        /* Simulation time in seconds. */
  bool pcapTracing = false;                          /* PCAP Tracing is enabled or not. */



  tcpVariant = std::string ("ns3::") + tcpVariant;
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));


  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_STANDARD_80211n);

  /* Set up Legacy Channel */
  YansWifiChannelHelper wifiChannel1;
  wifiChannel1.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel1.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy1;
  wifiPhy1.Set("ChannelSettings", StringValue("{36, 20, BAND_5GHZ, 0}"));
  
  wifiPhy1.SetChannel (wifiChannel1.Create ());
  wifiPhy1.SetErrorRateModel ("ns3::YansErrorRateModel");

  YansWifiChannelHelper wifiChannel2;
  wifiChannel2.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel2.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy2;
  wifiPhy2.SetChannel (wifiChannel1.Create ());
  wifiPhy2.Set("ChannelSettings", StringValue("{40, 20, BAND_5GHZ, 0}"));
  wifiPhy2.SetErrorRateModel ("ns3::YansErrorRateModel");




  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue (phyRate),
                                      "ControlMode", StringValue ("HtMcs0"));

  NodeContainer networkNodes;
  networkNodes.Create (5);
  Ptr<Node> ATL_apWifiNode1 = networkNodes.Get (0);
  Ptr<Node> CHI_apWifiNode2 = networkNodes.Get (1);
  Ptr<Node> staAWifiNode1 = networkNodes.Get (2);    // Node_A
  Ptr<Node> staBWifiNode2 = networkNodes.Get (3);    // Node_B
  Ptr<Node> DC_remoteServerNode = networkNodes.Get (4);
  
  NodeContainer staNodes;
  staNodes.Add (staAWifiNode1);
  staNodes.Add (staBWifiNode2);

  // Link between ATL_router and DC_remoteServerNode
  NodeContainer p2pNodeSet1;
  p2pNodeSet1.Add (ATL_apWifiNode1);
  p2pNodeSet1.Add (DC_remoteServerNode);

  // Link between ATL_router and CHI_router
  NodeContainer p2pNodeSet2;
  p2pNodeSet2.Add (ATL_apWifiNode1);
  p2pNodeSet2.Add (CHI_apWifiNode2);

  // Link between CHI_router and DC_remoteServerNode
  NodeContainer p2pNodeSet3;  
  p2pNodeSet3.Add (CHI_apWifiNode2);
  p2pNodeSet3.Add (DC_remoteServerNode);

  // Link between ATL_router and DC_remoteServerNode
  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("30ms"));  // p2pDelay1

  NetDeviceContainer p2pDevices1;
  p2pDevices1 = pointToPoint1.Install (p2pNodeSet1);

  // Link between ATL_router and CHI_router
  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("30ms"));  // p2pDelay2

  NetDeviceContainer p2pDevices2;
  p2pDevices2 = pointToPoint2.Install (p2pNodeSet2);

  // Link between CHI_router and DC_remoteServerNode
  PointToPointHelper pointToPoint3;
  pointToPoint3.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint3.SetChannelAttribute ("Delay", StringValue ("40ms"));    // p2pDelay3

  NetDeviceContainer p2pDevices3;
  p2pDevices3 = pointToPoint3.Install (p2pNodeSet3);

  /* Configure AP */
  Ssid ssid1 = Ssid ("network1");
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid1));

  NetDeviceContainer apDevice1;
  apDevice1 = wifiHelper.Install (wifiPhy1, wifiMac, ATL_apWifiNode1);

  Ssid ssid2 = Ssid ("network2");
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid2));

  NetDeviceContainer apDevice2;
  apDevice2 = wifiHelper.Install (wifiPhy2, wifiMac, CHI_apWifiNode2);

  /* Configure STA1 */
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid1));

  NetDeviceContainer staDevices1;
  staDevices1 = wifiHelper.Install (wifiPhy1, wifiMac, staAWifiNode1);

  /* Configure STA1 */
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid2));

  NetDeviceContainer staDevices2;
  staDevices2 = wifiHelper.Install (wifiPhy2, wifiMac, staBWifiNode2);

  /* Mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1000.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1000.0, 0.0, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ATL_apWifiNode1);
  mobility.Install (staAWifiNode1);
  mobility.Install (CHI_apWifiNode2);
  mobility.Install (staBWifiNode2);

  /* Internet stack */
  InternetStackHelper stack;
  stack.Install (networkNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address.Assign (p2pDevices1);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces2;
  p2pInterfaces2 = address.Assign (p2pDevices2);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces3;
  p2pInterfaces3 = address.Assign (p2pDevices3);

  address.SetBase ("10.2.1.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface1;
  apInterface1 = address.Assign (apDevice1);
  Ipv4InterfaceContainer staInterface1;
  staInterface1 = address.Assign (staDevices1);

  address.SetBase ("10.2.2.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface2;
  apInterface2 = address.Assign (apDevice2);
  Ipv4InterfaceContainer staInterface2;
  staInterface2 = address.Assign (staDevices2);



  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // RoutingTableModification - triangular routing
  // Modify the routing table with entries for a route
  // To node staBWifiNode2 from DC_remoteServerNode, next hop is ATL_apWifiNode1
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4> ipv4 = DC_remoteServerNode->GetObject<Ipv4> ();
  Ptr<Ipv4StaticRouting> staticRouting_DC = ipv4RoutingHelper.GetStaticRouting(ipv4);
  staticRouting_DC->AddHostRouteTo(Ipv4Address("10.2.2.2"), Ipv4Address("10.1.1.1"), 1);
  // Syntax: AddHostRouteTo(Ipv4Address destination, Ipv4Address nextHop, uint32_t interface)
  
  
  // IngressFilteringSolution -  Mitigate ingress filtering here - uncomment next 3 lines and appropriately complete the third line
  // Ptr<Ipv4> ipv4C = CHI_apWifiNode2->GetObject<Ipv4> ();
  // Ptr<Ipv4StaticRouting> staticRouting_CHI = ipv4RoutingHelper.GetStaticRouting(ipv4C);
  // staticRouting_CHI->AddHostRouteTo(Ipv4Address("10.x.x.x"), Ipv4Address("10.x.x.x"), 1);  // Line3: Complete this line. Do not change the interface number.

  


  /* Install TCP Receiver on the P2P node A */
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (DC_remoteServerNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));

  /* Install TCP/UDP Transmitter on the station */
  OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (p2pInterfaces1.GetAddress (1), 9)));
  server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  ApplicationContainer serverApp1 = server.Install (staAWifiNode1);
  ApplicationContainer serverApp2 = server.Install (staBWifiNode2);

  /* Start Applications */
  sinkApp.Start (Seconds (0.0));
  serverApp1.Start (Seconds (1.000));
  serverApp2.Start (Seconds (1.000));
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      // Enable pcap for p2pNodeSet1
      pointToPoint1.EnablePcapAll ("pa3_q2_1");
      // pointToPoint2.EnablePcapAll ("pa3_q2_2");
      // pointToPoint3.EnablePcapAll ("pa3_q2_3");
    }



  // Flow Monitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  /* Start Simulation */
  Simulator::Stop (Seconds (simulationTime + 1));

  // Trace routing tables
  Ptr<OutputStreamWrapper> routingStream =
      Create<OutputStreamWrapper>("pa3.routes", std::ios::out);
  // Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(7), routingStream);
  // Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(7), staNodes.Get(1), routingStream);
  Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(7), CHI_apWifiNode2, routingStream);
  Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(8), DC_remoteServerNode, routingStream);

  Simulator::Run ();

  double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));
  
  
  monitor->SerializeToXmlFile("pa3_1.flowmon", false, true);
  

  monitor->CheckForLostPackets ();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      if (true)
      {
          std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      	  std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
      }
     }



  monitor->SerializeToXmlFile("pa3_2.flowmon", false, true);

  Simulator::Destroy ();


  std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;
  return 0;
}
