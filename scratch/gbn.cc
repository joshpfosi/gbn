/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/gbn-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GBN Throughput");

double lastRx;

void RxTracer(Ptr< const Packet > packet, const Address &address)
{
    lastRx = Simulator::Now().GetSeconds();
}

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable("GbnNetDevice", LOG_LEVEL_INFO);
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (4294967295));

  const uint32_t RCVR = 1, SNDR = 0;

  std::string rate = "5Mbps", delay = "1ms";
  double errorRate = 0.0;
  uint64_t windowSize = 10;

  CommandLine cmd;
  cmd.AddValue("Rate", "Data rate of devices (R)", rate);
  cmd.AddValue("Delay", "Delay of channel (t_prop)", delay);
  cmd.AddValue("ErrorRate", "Receive error rate (P)", errorRate);
  cmd.AddValue("WindowSize", "Window size (W)", windowSize);
  cmd.Parse(argc, argv);

  NodeContainer gbnNodes;
  gbnNodes.Create(2);
  
  GbnNetDeviceHelper gbn; // TODO: GbnNetDeviceHelper
  gbn.SetDeviceAttribute("DataRate", StringValue (rate));
  gbn.SetChannelAttribute("Delay", StringValue (delay));


  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetUnit(ns3::RateErrorModel::ERROR_UNIT_PACKET);
  rem->SetRate(errorRate);

  gbn.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
  // gbn.SetDeviceAttribute ("WindowSize", UintegerValue (windowSize));

  NetDeviceContainer gbnDevices = gbn.Install(gbnNodes);

  // --------------------------------------------------------------------------
  // Install IP and assign IP addresses
  InternetStackHelper stack;
  stack.Install(gbnNodes);

  Ipv4AddressHelper address;

  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer gbnInterfaces;
  gbnInterfaces = address.Assign(gbnDevices);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Server
  uint16_t port = 9;
  OnOffHelper source("ns3::TcpSocketFactory",
          InetSocketAddress(gbnInterfaces.GetAddress(RCVR), port));
  source.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  source.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  source.SetAttribute("MaxBytes", UintegerValue(0));

  ApplicationContainer sourceApps = source.Install(gbnNodes.Get(SNDR));
  sourceApps.Start(Seconds (0.0));
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Client
  PacketSinkHelper sink("ns3::TcpSocketFactory",
                         InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer sinkApps = sink.Install(gbnNodes.Get(RCVR));
  sinkApps.Start(Seconds(0.0));

  // Enable tracing for last received packet
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
  sink1->TraceConnectWithoutContext("Rx", MakeCallback(&RxTracer));
  // --------------------------------------------------------------------------

  // Configure routing tables for all nodes
  // NOTE: Commented out an assertion here
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(10.0));

  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "Throughput: " << sink1->GetTotalRx() * 8 / lastRx << "bps" << std::endl;

  return 0;
}
