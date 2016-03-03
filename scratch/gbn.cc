/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/gbn-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GBN Throughput");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable("GbnSenderApplication", LOG_LEVEL_INFO);
  LogComponentEnable("GbnReceiverApplication", LOG_LEVEL_INFO);
  LogComponentEnable("GbnNetDevice", LOG_LEVEL_DEBUG);
  LogComponentEnable("Queue", LOG_LEVEL_INFO);
  LogComponentEnable("GbnChannel", LOG_LEVEL_INFO);
  LogComponentEnable("DataRate", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create(2);

  std::string rate = "5Mbps", delay = "2ms";
  double errorRate = 0.0;

  CommandLine cmd;
  cmd.AddValue("Rate", "Data rate of devices (R)", rate);
  cmd.AddValue("Delay", "Delay of channel (t_prop)", delay);
  cmd.AddValue("ErrorRate", "Receive error rate (P)", errorRate);
  cmd.Parse(argc, argv);

  GbnNetDeviceHelper GbnNetDevice;
  GbnNetDevice.SetDeviceAttribute("DataRate", StringValue (rate));
  GbnNetDevice.SetChannelAttribute("Delay", StringValue (delay));
  GbnNetDevice.SetNetDevicePointToPointMode(true);

  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetUnit(ns3::RateErrorModel::ERROR_UNIT_PACKET);
  rem->SetRate(errorRate);

  GbnNetDevice.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));

  GbnNetDevice.Install(nodes);

  Address rcvrAddr = nodes.Get(1)->GetDevice(0)->GetAddress(); // Mac48Address

  GbnReceiverHelper rcvr;

  ApplicationContainer receiverApps = rcvr.Install (nodes.Get (1));
  receiverApps.Start(Seconds (1.0));
  receiverApps.Stop(Seconds (1000));

  GbnSenderHelper sender(rcvrAddr);
  // TODO: parametrize on cmd line
  // sender.SetAttribute ("WindowSize", UintegerValue (1));

  ApplicationContainer senderApps = sender.Install (nodes.Get (0));
  senderApps.Start (Seconds (2.0));
  senderApps.Stop (Seconds (2.2));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
