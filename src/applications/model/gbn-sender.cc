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
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "gbn-sender.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GbnSenderApplication");

NS_OBJECT_ENSURE_REGISTERED (GbnSender);

TypeId
GbnSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GbnSender")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<GbnSender> ()
    .AddAttribute ("RcvrAddress", 
                   "MAC address of receiving device",
                   AddressValue(),
                   MakeAddressAccessor(&GbnSender::m_rcvr_addr),
                   MakeAddressChecker())
  ;
  return tid;
}

GbnSender::GbnSender ()
{
  NS_LOG_FUNCTION (this);
  m_dev = 0;
  m_sent = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_size = 1024;
}

GbnSender::~GbnSender()
{
  NS_LOG_FUNCTION (this);

  delete [] m_data;
  m_data = 0;
}

void 
GbnSender::SetRemote (Address mac)
{
  NS_LOG_FUNCTION (this << mac);
  m_rcvr_addr = mac;
}

void
GbnSender::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
GbnSender::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  if (m_dev == 0)
    {
        m_dev = GetNode()->GetDevice(0);

        DataRateValue rate;
        m_dev->GetAttribute("DataRate", rate);

        uint64_t data_rate = rate.Get().GetBitRate();
        (void)data_rate;
        m_interval = Seconds(0.001); //Seconds((double) m_size / (double) data_rate);
        // L / R => PacketSize / DataRate
    }

  m_dev->SetReceiveCallback(MakeCallback(&GbnSender::HandleRead, this));

  ScheduleTransmit(Seconds(0.));
}

void 
GbnSender::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_sendEvent);
}

void 
GbnSender::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &GbnSender::Send, this);
}

void 
GbnSender::Send (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> p = Create<Packet> (m_size);

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  NS_LOG_DEBUG("At time " << Simulator::Now ().GetSeconds () <<
          "s sender sent " << p->GetSize () << " bytes mac " << m_rcvr_addr);
  m_dev->Send(p, m_rcvr_addr, 0x0800); // IPv4
  // disregard return value -- use ACKs to determine success

  ++m_sent;

  NS_LOG_INFO("Sent packet " << m_sent << " at "
          << Simulator::Now().GetSeconds());

  ScheduleTransmit (m_interval);
}

bool 
GbnSender::HandleRead (Ptr<NetDevice> dev, Ptr<const Packet> p,
        uint16_t protocol, const Address &mac)
{
    NS_LOG_FUNCTION (this << dev << p << protocol << mac);

    uint16_t ack_no;
    p->CopyData((uint8_t *)&ack_no, sizeof(ack_no));

    // NS_LOG_DEBUG("At time " << Simulator::Now ().GetSeconds () <<
    //         "s sender received " << ack_no << " " << p->GetSize () << " bytes mac " << mac);
    return true;
}

} // Namespace ns3
