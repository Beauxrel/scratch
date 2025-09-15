/*
 * Copyright (c) 2015, IMDEA Networks Institute
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hany Assasa <hany.assasa@gmail.com>
.*
 * This is a simple example to test TCP over 802.11n (with MPDU aggregation enabled).
 *
 * Network topology:
 *
 *   Ap    STA
 *   *      *
 *   |      |
 *   n1     n2
 *
 * In this example, an HT station sends TCP packets to the access point.
 * We report the total throughput received during a window of 100ms.
 * The user can specify the application data rate and choose the variant
 * of TCP i.e. congestion control algorithm to use.
 */

 /*
 * ECE 6610 Fall 2025
 * Instructor: Prof. Karthikeyan Sundaresan
 * Programming Assignment 1
 * Q2 - TCP connection over WiFi
 * Team Members: <<Your Names Here>>
 * Team Number: <<Your Team Number Here>>
 * Date: <<Date Here>>
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/tcp-westwood-plus.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

NS_LOG_COMPONENT_DEFINE("wifi-tcp");

using namespace ns3;

Ptr<PacketSink> sink1;     //!< Pointer to the packet sink application for STA1
Ptr<PacketSink> sink2;     //!< Pointer to the packet sink application for STA2
uint64_t lastTotalRx1 = 0; //!< The value of the last total received bytes for STA1
uint64_t lastTotalRx2 = 0; //!< The value of the last total received bytes for STA2


/**
 * Calculate the throughput
 */
void
CalculateThroughput()
{
    Time now = Simulator::Now(); /* Return the simulator's virtual time. */

    // Calculate the throughput for STA1 and STA2
    double cur1 = (sink1->GetTotalRx() - lastTotalRx1) * 8.0 / 1e5; /* Convert Application RX Packets to MBits. */
    std::cout << now.GetSeconds() << "s: \t" << cur1 << " Mbit/s" << std::endl;
    lastTotalRx1 = sink1->GetTotalRx();

    double cur2 = (sink2->GetTotalRx() - lastTotalRx2) * 8.0 / 1e5; /* Convert Application RX Packets to MBits. */
    std::cout << now.GetSeconds() << "s: \t" << cur2 << " Mbit/s" << std::endl;
    lastTotalRx2 = sink2->GetTotalRx();

    // Calculate the total throughput
    double totalThroughput = cur1 + cur2;

    std::cout << now.GetSeconds() << "s: \tSTA1: " << cur1 << " Mbit/s\tSTA2: " << cur2 << " Mbit/s\tTotal: " << totalThroughput << " Mbit/s" << std::endl;

    Simulator::Schedule(MilliSeconds(100), &CalculateThroughput);
}

int
main(int argc, char* argv[])
{
    uint32_t payloadSize = 1472;           /* Transport layer payload size in bytes. */
    std::string dataRate = "100Mbps";      /* Application layer datarate. */
    std::string tcpVariant = "TcpNewReno"; /* TCP variant type. */
    std::string phyRate = "HtMcs7";        /* Physical layer bitrate -- Determines maximum possible physical layer rate */
    double simulationTime = 10;            /* Simulation time in seconds. */
    bool pcapTracing = false;              /* PCAP Tracing is enabled or not. */
    bool enableLargeAmpdu = false;               /* Enable/disable A-MPDU */
    bool enableRts = false;               /* Enable/disable CTS/RTS */
    std::string frequencyBand = "5GHz";     /* Set to '5GHz or '2_4GHz' ;  Frequency band to use */

    /* Command line argument parser setup. */
    CommandLine cmd(__FILE__);
    cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue("dataRate", "Application data rate", dataRate);
    cmd.AddValue("tcpVariant",
                 "Transport protocol to use: TcpNewReno, "
                 "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                 "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ",
                 tcpVariant);
    cmd.AddValue("phyRate", "Physical layer bitrate", phyRate);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue("pcap", "Enable/disable PCAP Tracing", pcapTracing);
    cmd.AddValue("enableLargeAmpdu", "Enable/disable A-MPDU", enableLargeAmpdu);
    cmd.AddValue("enableRts", "Enable/disable RTS/CTS", enableRts);
    cmd.Parse(argc, argv);

    tcpVariant = std::string("ns3::") + tcpVariant;
    // Select TCP variant
    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(tcpVariant, &tcpTid),
                        "TypeId " << tcpVariant << " not found");
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(tcpVariant)));

    /* Configure TCP Options */
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));

    if (enableRts)
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(0));
    }
    else
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(999999));
    }
    WifiMacHelper wifiMac;
    WifiHelper wifiHelper;
    wifiHelper.SetStandard(WIFI_STANDARD_80211n);

    /* Set up Legacy Channel */
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    if (frequencyBand == "5GHz")
    {
        wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue(5e9));
    }
    else if (frequencyBand == "2_4GHz")
    {
        wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue(2.4e9));
    }
    else
    {
        std::cout << "Invalid frequency band. Please set to '5GHz' or '2_4GHz'" << std::endl;
        return 1;
    }
    

    /* Setup Physical Layer */
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");
    if (frequencyBand == "5GHz")
    {
        wifiPhy.Set("ChannelSettings", StringValue("{36, 0, BAND_5GHZ, 0}"));
    }
    else if (frequencyBand == "2_4GHz")
    {
        wifiPhy.Set("ChannelSettings", StringValue("{1, 0, BAND_2_4GHZ, 0}"));
    }
    else
    {
        std::cout << "Invalid frequency band. Please set to '5GHz' or '2_4GHz'" << std::endl;
        return 1;
    }
    
    wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                       "DataMode",
                                       StringValue(phyRate),
                                       "ControlMode",
                                       StringValue("HtMcs0"));
    /* Create 3 nodes. 1 AP and 2 STAs */
    NodeContainer networkNodes;
    networkNodes.Create(3);
    Ptr<Node> apWifiNode = networkNodes.Get(0);
    Ptr<Node> sta1WifiNode = networkNodes.Get(1);
    Ptr<Node> sta2WifiNode = networkNodes.Get(2);

    /* Configure AP */
    Ssid ssid = Ssid("network");
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice;
    apDevice = wifiHelper.Install(wifiPhy, wifiMac, apWifiNode);

    /* Configure STAs */
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer staDevices;
    staDevices = wifiHelper.Install(wifiPhy, wifiMac, sta1WifiNode);
    staDevices.Add(wifiHelper.Install(wifiPhy, wifiMac, sta2WifiNode));

    if (!enableLargeAmpdu)
    {
        Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_MaxAmpduSize", UintegerValue (4000)); 
    }
    

    /* Mobility model */
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));      // AP position
    positionAlloc->Add(Vector(-10.0, 0.0, 0.0));     // STA1 position - pay attention to distance calculation. Change just one coordinate for simple calculation
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));     // STA2 position - pay attention to distance calculation. Change just one coordinate for simple calculation

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(networkNodes);
    /*
    mobility.Install(apWifiNode);                   // Position is assigned here - order of positionAlloc is important
    mobility.Install(staWifiNode);
    */


    /* Internet stack */
    InternetStackHelper stack;
    stack.Install(networkNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevice);
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);

    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    /* Install TCP Receiver on the access point -Port 9  for STA1  and Port 10 for STA2 */
    PacketSinkHelper sinkHelper1("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer sinkApp1 = sinkHelper1.Install(apWifiNode);
    sink1 = StaticCast<PacketSink>(sinkApp1.Get(0));

    PacketSinkHelper sinkHelper2("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), 10));
    ApplicationContainer sinkApp2 = sinkHelper2.Install(apWifiNode);
    sink2 = StaticCast<PacketSink>(sinkApp2.Get(0));

    /* Install TCP Transmitter on the STA1 */
    OnOffHelper server1("ns3::TcpSocketFactory", (InetSocketAddress(apInterface.GetAddress(0), 9)));
    server1.SetAttribute("PacketSize", UintegerValue(payloadSize));
    server1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    server1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    server1.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    ApplicationContainer serverApp1 = server1.Install(sta1WifiNode);

    /* Install TCP Transmitter on the STA2 */
    OnOffHelper server2("ns3::TcpSocketFactory", (InetSocketAddress(apInterface.GetAddress(0), 10)));
    server2.SetAttribute("PacketSize", UintegerValue(payloadSize));
    server2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    server2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    server2.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    ApplicationContainer serverApp2 = server2.Install(sta2WifiNode);

    /* Start Applications */
    sinkApp1.Start(Seconds(0.0));
    sinkApp2.Start(Seconds(0.0));
    serverApp1.Start(Seconds(1.0));
    serverApp2.Start(Seconds(1.0));
    Simulator::Schedule(Seconds(1.1), &CalculateThroughput);

    /* Enable Traces */
    if (pcapTracing)
    {
        wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        wifiPhy.EnablePcap("AccessPoint", apDevice);
        wifiPhy.EnablePcap("Station1", staDevices.Get(0));
        wifiPhy.EnablePcap("Station2", staDevices.Get(1));
        /*
        wifiPhy.EnablePcap("module2-AccessPoint", apDevice);
        wifiPhy.EnablePcap("module2-Station", staDevices);
        */
    }

    /* Start Simulation */
    Simulator::Stop(Seconds(simulationTime + 1));
    Simulator::Run();

    double averageThroughput1 = ((sink1->GetTotalRx() * 8) / (1e6 * simulationTime));
    double averageThroughput2 = ((sink2->GetTotalRx() * 8) / (1e6 * simulationTime));

    Simulator::Destroy();

    std::cout << "\nAverage throughput STA1: " << averageThroughput1 << " Mbit/s" << std::endl;
    std::cout << "\nAverage throughput STA2: " << averageThroughput2 << " Mbit/s" << std::endl;
    std::cout << "\nAverage total throughput: " << averageThroughput1 + averageThroughput2 << " Mbit/s" << std::endl;
    std::cout << "enableRts: " << (enableRts ? "true" : "false") << std::endl;
    return 0;

}
