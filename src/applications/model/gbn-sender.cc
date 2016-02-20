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
    .AddAttribute ("RcvrMacAddress", 
                   "MAC address of receiving device",
                   AddressValue(),
                   MakeAddressAccessor(&GbnSender::m_rcvr_addr),
                   MakeAddressChecker())
    .AddAttribute ("PacketSize", "Size of data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&GbnSender::SetDataSize,
                                         &GbnSender::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
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
  m_dataSize = 0;
  m_count = 5;
}

GbnSender::~GbnSender()
{
  NS_LOG_FUNCTION (this);

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
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
    }

  m_dev->SetReceiveCallback(MakeCallback(&GbnSender::HandleRead, this));

  ScheduleTransmit (Seconds (0.));
}

void 
GbnSender::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_sendEvent);
}

void 
GbnSender::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so 
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t 
GbnSender::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
GbnSender::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
GbnSender::SetFill (uint8_t fill, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
GbnSender::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      m_size = dataSize;
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
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

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "GbnSender::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "GbnSender::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that it doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding attribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
    }
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  m_dev->Send(p, m_rcvr_addr, 0x0800); // IPv4
  // disregard return value -- use ACKs to determine success

  ++m_sent;

  if (m_sent < m_count) 
    {
      ScheduleTransmit (m_interval);
    }
}

bool 
GbnSender::HandleRead (Ptr<NetDevice> dev, Ptr<const Packet> p,
        uint16_t protocol, const Address &mac)
{
    NS_LOG_FUNCTION (this << dev << p << protocol << mac);
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () <<
            "s sender received " << p->GetSize () << " bytes mac " << mac);
    return true;
}

} // Namespace ns3
