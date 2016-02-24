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
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GBN Throughput");

int
main (int argc, char *argv[])
{
  // TODO: Add CommandLine parser
  Time::SetResolution (Time::NS);
  LogComponentEnable("GbnSenderApplication", LOG_LEVEL_INFO);
  LogComponentEnable("GbnReceiverApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create(2);

  std::string rate = "5Mbps", delay = "2ms";
  double errorRate = 0.0;

  CommandLine cmd;
  cmd.AddValue("Rate", "Data rate of devices", rate);
  cmd.AddValue("Delay", "Delay of channel", delay);
  cmd.AddValue("ErrorRate", "Receive error rate", errorRate);
  cmd.Parse(argc, argv);

  SimpleNetDeviceHelper simpleNetDevice;
  simpleNetDevice.SetDeviceAttribute("DataRate", StringValue (rate));
  simpleNetDevice.SetChannelAttribute("Delay", StringValue (delay));
  simpleNetDevice.SetNetDevicePointToPointMode(true);

  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetUnit(ns3::RateErrorModel::ERROR_UNIT_PACKET);
  rem->SetRate(errorRate);

  simpleNetDevice.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));

  simpleNetDevice.Install(nodes);

  Address rcvrAddr = nodes.Get(1)->GetDevice(0)->GetAddress(); // Mac48Address

  GbnReceiverHelper rcvr;

  ApplicationContainer receiverApps = rcvr.Install (nodes.Get (1));
  receiverApps.Start(Seconds (1.0));
  receiverApps.Stop(Seconds (3.5));

  GbnSenderHelper sender(rcvrAddr);
  // TODO: parametrize on cmd line
  // sender.SetAttribute ("WindowSize", UintegerValue (1));

  ApplicationContainer senderApps = sender.Install (nodes.Get (0));
  senderApps.Start (Seconds (2.0));
  senderApps.Stop (Seconds (2.5));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
