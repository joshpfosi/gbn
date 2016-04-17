/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

double lastRx;

void RxTracer(Ptr< const Packet > packet, const Address &address)
{
    lastRx = Simulator::Now().GetSeconds();
}

int 
main (int argc, char *argv[])
{
  const uint32_t nCsma    = 3;
  const uint32_t nWifi    = 3;
  const uint32_t WIFI     = 1;
  const uint32_t ETHERNET = 0;

  double errorRate      = 0.0;
  uint32_t maxBytes     = 1 << 16;
  bool verbose          = true;
  bool tracing          = false;
  std::string gbnRate   = "5Mbps";
  std::string gbnDelay  = "2ms";
  std::string satRate   = "1Kbps";
  std::string satDelay  = "200ms";
  std::string csmaRate  = "5Mbps";
  std::string csmaDelay = "2ms";

  CommandLine cmd;

  cmd.AddValue ("verbose",  "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing",  "Enable pcap tracing",                   tracing);
  cmd.AddValue("ErrorRate", "Receive error rate (P)",                errorRate);
  cmd.AddValue("GbnRate",   "Data rate of GBN devices (R)",          gbnRate);
  cmd.AddValue("GbnDelay",  "Delay of GBN channel (t_prop)",         gbnDelay);
  cmd.AddValue("SatRate",   "Data rate of sat devices (R)",          satRate);
  cmd.AddValue("SatDelay",  "Delay of sat channel (t_prop)",         satDelay);
  cmd.AddValue("CsmaRate",  "Data rate of csma devices (R)",         csmaRate);
  cmd.AddValue("CsmaDelay", "Delay of csma channel (t_prop)",        csmaDelay);
  // TODO: Change traffic distribution
  // TODO: Change error distribution

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  // --------------------------------------------------------------------------
  // Border routers
  NodeContainer borderNodes;
  borderNodes.Create (2);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // GBN links
  NodeContainer gbnNodes;
  gbnNodes.Create (5);

  PointToPointHelper gbn; // TODO: Should use GBN links
  gbn.SetDeviceAttribute("DataRate", StringValue(gbnRate));
  gbn.SetChannelAttribute("Delay",   StringValue(gbnDelay));

  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetUnit(ns3::RateErrorModel::ERROR_UNIT_PACKET);
  rem->SetRate(errorRate);

  gbn.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));

  NetDeviceContainer gbnDevices;

  gbnDevices.Add(gbn.Install(borderNodes.Get(ETHERNET), gbnNodes.Get(0)));
  gbnDevices.Add(gbn.Install(borderNodes.Get(ETHERNET), gbnNodes.Get(1)));
  gbnDevices.Add(gbn.Install(borderNodes.Get(ETHERNET), gbnNodes.Get(2)));
  gbnDevices.Add(gbn.Install(borderNodes.Get(WIFI),     gbnNodes.Get(0)));
  gbnDevices.Add(gbn.Install(borderNodes.Get(WIFI),     gbnNodes.Get(1)));
  gbnDevices.Add(gbn.Install(borderNodes.Get(WIFI),     gbnNodes.Get(3)));
  gbnDevices.Add(gbn.Install(gbnNodes.Get(2),           gbnNodes.Get(1)));
  gbnDevices.Add(gbn.Install(gbnNodes.Get(2),           gbnNodes.Get(4)));
  gbnDevices.Add(gbn.Install(gbnNodes.Get(3),           gbnNodes.Get(1)));
  gbnDevices.Add(gbn.Install(gbnNodes.Get(3),           gbnNodes.Get(4)));
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Satellite links
  NodeContainer satNodes;
  satNodes.Create (1);

  PointToPointHelper satellite;
  satellite.SetDeviceAttribute("DataRate", StringValue(satRate));
  satellite.SetChannelAttribute("Delay", StringValue(satDelay));
  // Satellite link is SLOW

  gbnDevices.Add(satellite.Install(borderNodes.Get(ETHERNET), satNodes.Get(0)));
  gbnDevices.Add(satellite.Install(satNodes.Get(0), borderNodes.Get(WIFI)));
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Ethernet links
  NodeContainer csmaNodes;
  csmaNodes.Add(borderNodes.Get(ETHERNET));
  csmaNodes.Create(nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue(csmaRate));
  csma.SetChannelAttribute("Delay", StringValue(csmaDelay));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install(csmaNodes);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // WiFi links
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(nWifi);
  NodeContainer wifiApNode = borderNodes.Get(WIFI);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetChannel(channel.Create());

  WifiHelper wifi = WifiHelper::Default();
  wifi.SetRemoteStationManager("ns3::AarfWifiManager");

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default();

  Ssid ssid = Ssid("ns-3-ssid");
  mac.SetType("ns3::StaWifiMac",
               "Ssid", SsidValue(ssid),
               "ActiveProbing", BooleanValue(false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install(phy, mac, wifiStaNodes);

  mac.SetType("ns3::ApWifiMac",
               "Ssid", SsidValue(ssid));

  // WiFi AP
  NetDeviceContainer apDevices;
  apDevices = wifi.Install(phy, mac, wifiApNode);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Make WiFi nodes move
  MobilityHelper mobility;

  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
          "MinX", DoubleValue (0.0),
          "MinY", DoubleValue (0.0),
          "DeltaX", DoubleValue (5.0),
          "DeltaY", DoubleValue (10.0),
          "GridWidth", UintegerValue (3),
          "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Bounds",
          RectangleValue(Rectangle(-50, 50, -50, 50)));
  mobility.Install(wifiStaNodes);

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiApNode);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Install IP and assign IP addresses
  InternetStackHelper stack;
  stack.Install(csmaNodes);
  stack.Install(wifiApNode);
  stack.Install(wifiStaNodes);
  stack.Install(gbnNodes);
  stack.Install(satNodes);
  // NOTE: borderNodes are assigned via [csma,wifiAp]Nodes

  Ipv4AddressHelper address;

  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer gbnInterfaces;
  gbnInterfaces = address.Assign(gbnDevices);

  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign(csmaDevices);

  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiAddresses = address.Assign(staDevices);
  address.Assign(apDevices);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Server
  uint16_t port = 9;
  OnOffHelper source("ns3::TcpSocketFactory",
          InetSocketAddress(wifiAddresses.GetAddress(nWifi - 1), port));
  source.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  source.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  source.SetAttribute("MaxBytes", UintegerValue(maxBytes));

  ApplicationContainer sourceApps = source.Install(csmaNodes.Get(nCsma));
  sourceApps.Start(Seconds (0.0));
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Client
  PacketSinkHelper sink("ns3::TcpSocketFactory",
                         InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer sinkApps = sink.Install(wifiStaNodes.Get(nWifi - 1));
  sinkApps.Start(Seconds(0.0));

  // Enable tracing for last received packet
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
  sink1->TraceConnectWithoutContext("Rx", MakeCallback(&RxTracer));
  // --------------------------------------------------------------------------

  // Configure routing tables for all nodes
  // NOTE: Commented out an assertion here
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  // TODO: May want to use static routing
  // (https://www.nsnam.org/doxygen/static-routing-slash32_8cc_source.html)

  Simulator::Stop(Seconds(10.0));

  if (tracing == true)
    {
      gbn.EnablePcapAll("star");
      phy.EnablePcap("star", apDevices.Get(0));
      csma.EnablePcap("star", csmaDevices.Get(0), true);
    }

  Simulator::Run();
  Simulator::Destroy();

  std::cout << "Throughput: " << sink1->GetTotalRx() * 8 / lastRx << "bps" << std::endl;

  return 0;
}
