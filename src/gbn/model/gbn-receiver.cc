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
#include "ns3/mac48-address.h"
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

GbnReceiver::GbnReceiver () :
    m_dev(0),
    m_bytes_rx(0),
    m_last_rx(0)
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

  if (m_dev == 0)
    {
        m_dev = GetNode()->GetDevice(0);
    }

  m_dev->SetReceiveCallback(MakeCallback(&GbnReceiver::HandleRead, this));
}

void 
GbnReceiver::StopApplication (void)
{
    NS_LOG_FUNCTION (this);
    size_t throughput = (m_last_rx != 0) ? m_bytes_rx * 8 / (m_last_rx - 2) : 0;
    NS_LOG_DEBUG("RECEIVED " << m_bytes_rx << " bytes");
    NS_LOG_DEBUG("THROUGHPUT " << throughput << " bps");
}

bool 
GbnReceiver::HandleRead (Ptr<NetDevice> dev, Ptr<const Packet> p,
        uint16_t protocol, const Address &mac)
{
    NS_LOG_FUNCTION (this);

    m_last_rx = Simulator::Now().GetSeconds();
    m_bytes_rx += p->GetSize();

    return true;
}

} // Namespace ns3
