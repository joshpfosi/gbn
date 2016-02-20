/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "gbn-receiver.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GbnReceiverApplication");

NS_OBJECT_ENSURE_REGISTERED (GbnReceiver);

TypeId
GbnReceiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GbnReceiver")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<GbnReceiver> ()
  ;
  return tid;
}

GbnReceiver::GbnReceiver ()
{
  NS_LOG_FUNCTION (this);
}

GbnReceiver::~GbnReceiver()
{
  NS_LOG_FUNCTION (this);
}

void
GbnReceiver::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
GbnReceiver::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  GetNode()->GetDevice(0)->SetReceiveCallback(MakeCallback(&GbnReceiver::HandleRead, this));
}

void 
GbnReceiver::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
}

bool 
GbnReceiver::HandleRead (Ptr<NetDevice> dev, Ptr<const Packet> p,
        uint16_t protocol, const Address &mac)
{
  NS_LOG_FUNCTION (this);

  // Ptr<Packet> packet;
  // Address from;
  // while ((packet = socket->RecvFrom (from)))
  //   {
  //     if (InetSocketAddress::IsMatchingType (from))
  //       {
  //         NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " <<
  //                      InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
  //                      InetSocketAddress::ConvertFrom (from).GetPort ());
  //       }
  //     else if (Inet6SocketAddress::IsMatchingType (from))
  //       {
  //         NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " <<
  //                      Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
  //                      Inet6SocketAddress::ConvertFrom (from).GetPort ());
  //       }

  //     packet->RemoveAllPacketTags ();
  //     packet->RemoveAllByteTags ();

  //     NS_LOG_LOGIC ("Echoing packet");
  //     socket->SendTo (packet, 0, from);

  //     if (InetSocketAddress::IsMatchingType (from))
  //       {
  //         NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server sent " << packet->GetSize () << " bytes to " <<
  //                      InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
  //                      InetSocketAddress::ConvertFrom (from).GetPort ());
  //       }
  //     else if (Inet6SocketAddress::IsMatchingType (from))
  //       {
  //         NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server sent " << packet->GetSize () << " bytes to " <<
  //                      Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
  //                      Inet6SocketAddress::ConvertFrom (from).GetPort ());
  //       }
  //   }
  return false;
}

} // Namespace ns3
