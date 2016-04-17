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

int 
main (int argc, char *argv[])
{
  bool verbose             = true;
  const uint32_t nCsma     = 3;
  const uint32_t nWifi     = 3;
  const uint32_t WIFI_AP   = 1;
  const uint32_t ETHERNET  = 0;
  const uint32_t SATELLITE = 0;
  bool tracing             = false;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  // --------------------------------------------------------------------------
  // P2P links
  NodeContainer p2pNodes, gbnNodes;
  p2pNodes.Create (2);
  gbnNodes.Create (6);

  PointToPointHelper pointToPoint;

  // GBN links
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // TODO: Should use GBN links
  NetDeviceContainer p2pDevices;
  p2pDevices.Add(pointToPoint.Install(p2pNodes.Get(ETHERNET), gbnNodes.Get(1)));
  p2pDevices.Add(pointToPoint.Install(p2pNodes.Get(ETHERNET), gbnNodes.Get(2)));
  p2pDevices.Add(pointToPoint.Install(p2pNodes.Get(ETHERNET), gbnNodes.Get(3)));
  p2pDevices.Add(pointToPoint.Install(p2pNodes.Get(WIFI_AP), gbnNodes.Get(1)));
  p2pDevices.Add(pointToPoint.Install(p2pNodes.Get(WIFI_AP), gbnNodes.Get(3)));
  p2pDevices.Add(pointToPoint.Install(p2pNodes.Get(WIFI_AP), gbnNodes.Get(4)));
  p2pDevices.Add(pointToPoint.Install(gbnNodes.Get(2), gbnNodes.Get(3)));
  p2pDevices.Add(pointToPoint.Install(gbnNodes.Get(2), gbnNodes.Get(5)));
  p2pDevices.Add(pointToPoint.Install(gbnNodes.Get(3), gbnNodes.Get(4)));
  p2pDevices.Add(pointToPoint.Install(gbnNodes.Get(4), gbnNodes.Get(5)));

  // Satellite links
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("100ms"));
  // Satellite link is SLOW

  p2pDevices.Add(pointToPoint.Install(p2pNodes.Get(ETHERNET),
              gbnNodes.Get(SATELLITE)));
  p2pDevices.Add(pointToPoint.Install(gbnNodes.Get(SATELLITE),
              p2pNodes.Get(WIFI_AP)));
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Ethernet links
  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (ETHERNET));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // WiFi links
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (WIFI_AP);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  // WiFi AP
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Make WiFi nodes move
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Install IP and assign IP addresses
  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);
  stack.Install (gbnNodes);
  // NOTE: p2pNodes are assigned via [csma,wifiAp]Nodes

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);
  // TODO: Assign addresses to GBN links

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Set up applications
  UdpEchoServerHelper echoServer (9); // port 9

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  // --------------------------------------------------------------------------

  // Configure routing tables for all nodes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // TODO: May want to use static routing
  // (https://www.nsnam.org/doxygen/static-routing-slash32_8cc_source.html)

  Simulator::Stop (Seconds (10.0));

  if (tracing == true)
    {
      pointToPoint.EnablePcapAll ("third");
      phy.EnablePcap ("third", apDevices.Get (0));
      csma.EnablePcap ("third", csmaDevices.Get (0), true);
    }

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
