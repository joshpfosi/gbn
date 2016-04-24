/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "gbn-net-device.h"
#include "gbn-channel.h"
#include "gbn-header.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/tag.h"
#include "ns3/simulator.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GbnNetDevice");

/**
 * \brief GbnNetDevice tag to store source, destination and protocol of each packet.
 */
class GbnTag : public Tag {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);

  /**
   * Set the source address
   * \param src source address
   */
  void SetSrc (Mac48Address src);
  /**
   * Get the source address
   * \return the source address
   */
  Mac48Address GetSrc (void) const;

  /**
   * Set the destination address
   * \param dst destination address
   */
  void SetDst (Mac48Address dst);
  /**
   * Get the destination address
   * \return the destination address
   */
  Mac48Address GetDst (void) const;

  /**
   * Set the protocol number
   * \param proto protocol number
   */
  void SetProto (uint16_t proto);
  /**
   * Get the protocol number
   * \return the protocol number
   */
  uint16_t GetProto (void) const;

  void Print (std::ostream &os) const;

private:
  Mac48Address m_src; //!< source address
  Mac48Address m_dst; //!< destination address
  uint16_t m_protocolNumber; //!< protocol number
};

NS_OBJECT_ENSURE_REGISTERED (GbnTag);

TypeId
GbnTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GbnTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<GbnTag> ()
  ;
  return tid;
}
TypeId
GbnTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
GbnTag::GetSerializedSize (void) const
{
  return 8+8+2;
}
void
GbnTag::Serialize (TagBuffer i) const
{
  uint8_t mac[6];
  m_src.CopyTo (mac);
  i.Write (mac, 6);
  m_dst.CopyTo (mac);
  i.Write (mac, 6);
  i.WriteU16 (m_protocolNumber);
}
void
GbnTag::Deserialize (TagBuffer i)
{
  uint8_t mac[6];
  i.Read (mac, 6);
  m_src.CopyFrom (mac);
  i.Read (mac, 6);
  m_dst.CopyFrom (mac);
  m_protocolNumber = i.ReadU16 ();
}

void
GbnTag::SetSrc (Mac48Address src)
{
  m_src = src;
}

Mac48Address
GbnTag::GetSrc (void) const
{
  return m_src;
}

void
GbnTag::SetDst (Mac48Address dst)
{
  m_dst = dst;
}

Mac48Address
GbnTag::GetDst (void) const
{
  return m_dst;
}

void
GbnTag::SetProto (uint16_t proto)
{
  m_protocolNumber = proto;
}

uint16_t
GbnTag::GetProto (void) const
{
  return m_protocolNumber;
}

void
GbnTag::Print (std::ostream &os) const
{
  os << "src=" << m_src << " dst=" << m_dst << " proto=" << m_protocolNumber;
}

NS_OBJECT_ENSURE_REGISTERED (GbnNetDevice);

TypeId 
GbnNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GbnNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("Network") 
    .AddConstructor<GbnNetDevice> ()
    .AddAttribute ("ReceiveErrorModel",
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&GbnNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("PointToPointMode",
                   "The device is configured in Point to Point mode",
                   BooleanValue (false),
                   MakeBooleanAccessor (&GbnNetDevice::m_pointToPointMode),
                   MakeBooleanChecker ())
    .AddAttribute ("TxQueue",
                   "A queue to use as the transmit queue in the device.",
                   StringValue ("ns3::DropTailQueue"),
                   MakePointerAccessor (&GbnNetDevice::m_queue),
                   MakePointerChecker<Queue> ())
    .AddAttribute ("WindowSize",
                   "The window size used in GBN ARQ",
                   UintegerValue (10),
                   MakeUintegerAccessor (&GbnNetDevice::m_wsize),
                   MakeUintegerChecker<size_t>())
    .AddAttribute ("DataRate",
                   "The default data rate for point to point links. Zero means infinite",
                   DataRateValue (DataRate ("0b/s")),
                   MakeDataRateAccessor (&GbnNetDevice::m_bps),
                   MakeDataRateChecker ())
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been dropped "
                     "by the device during reception",
                     MakeTraceSourceAccessor (&GbnNetDevice::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

GbnNetDevice::GbnNetDevice ()
  : m_channel (0),
    m_node (0),
    m_mtu (0xffff),
    m_ifIndex (0),
    m_linkUp (false),
    m_wsize (10),
    m_window(),
    timeoutTime(Seconds(1)),
    m_expected_seqno(0),
    m_seqno(0),
    m_max_seqno(65536)
{
  NS_LOG_FUNCTION (this);

  m_window.reserve(m_wsize);
  m_inflight = m_window.begin();
}

void
GbnNetDevice::Receive (Ptr<Packet> packet, uint16_t protocol,
                          Mac48Address to, Mac48Address from)
{
    NS_LOG_FUNCTION (this << packet << protocol << to << from);
    NetDevice::PacketType packetType;

    if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
        m_phyRxDropTrace (packet);
        return;
    }

    if (to == m_address)
    {
        packetType = NetDevice::PACKET_HOST;
    }
    else if (to.IsBroadcast ())
    {
        packetType = NetDevice::PACKET_BROADCAST;
    }
    else if (to.IsGroup ())
    {
        packetType = NetDevice::PACKET_MULTICAST;
    }
    else 
    {
        packetType = NetDevice::PACKET_OTHERHOST;
    }
    GbnHeader h; packet->PeekHeader(h); // debugging
    NS_LOG_DEBUG("[RECEIVE] Received packet seqno=" << h.GetSeqno()
            << " of type " << packetType);

    if (packetType != NetDevice::PACKET_OTHERHOST)
    {
        GbnHeader header;
        packet->RemoveHeader(header);

        if (header.GetIsAck()) // Sender received an ACK
        {
            NS_LOG_DEBUG("[RECEIVE] (Sender) Received ACK for seqno="
                    << header.GetSeqno());
            Ptr<Packet> p = m_window.begin()->first; p->PeekHeader(h);
            NS_LOG_DEBUG("[RECEIVE] (Sender) Erasing seqno=" << h.GetSeqno());

            m_window.begin()->second.Cancel(); // cancel timeout
            m_window.erase(m_window.begin()); // clear packet
            --m_inflight; // `erase` invalidates iterators

            // Enqueue new packet if one exists
            NS_LOG_DEBUG("[RECEIVE] (Sender) m_queue.size()="
                    << m_queue->GetNPackets());
            if (m_queue->GetNPackets())
            {
                Ptr<Packet> dataPacket = m_queue->Dequeue();
                PacketPair packetPair = std::make_pair(dataPacket, EventId());

                // Schedule a TransmitComplete event if necessary
                if (!TransmitCompleteEvent.IsRunning ())
                {
                    Time txTime = Time (0);
                    if (m_bps > DataRate (0))
                    {
                        txTime = m_bps.CalculateBytesTxTime (dataPacket->GetSize ());
                    }

                    NS_ASSERT(txTime < timeoutTime);

                    packetPair.second = Simulator::Schedule (timeoutTime,
                            &GbnNetDevice::Timeout, this);
                    TransmitCompleteEvent = Simulator::Schedule (txTime,
                            &GbnNetDevice::TransmitComplete, this);
                    GbnHeader h; dataPacket->PeekHeader(h);
                    NS_LOG_DEBUG("[RECEIVE] (Sender) Scheduling packet "
                            << h.GetSeqno() << " for "
                            << Simulator::Now().GetSeconds() + txTime.GetSeconds());
                }

                NS_LOG_DEBUG("[RECEIVE] (Sender) Pushing onto window of size "
                        << m_window.size());
                // NOTE: Window can always hold at least 1 more element as we
                // erased one above
                m_window.push_back(packetPair);

                // Shift window if necessary
                if (m_inflight == m_window.end())
                {
                    NS_LOG_DEBUG("[RECEIVE] (Sender) Shifting window");
                    m_inflight = --m_window.end();
                }
            }
        }
        else // Receiver got a packet so ACK
        {
            GbnHeader h;
            h.SetIsAck (true);
            h.SetSeqno (m_expected_seqno);

            Ptr<Packet> ack = Create<Packet>(0);
            ack->AddHeader(h);

            // SendFrom is more realistic but m_channel->Send() is probably
            // required b/c the analytical models do not account for ACKs
            // transmission delay
            NS_LOG_DEBUG("[RECEIVE] (Receiver) Sending ACK for seqno="
                    << m_expected_seqno);
            m_channel->Send(ack, protocol, from, m_address, this);

            if (header.GetSeqno() == m_expected_seqno) // expected, so increment
            {
                NS_LOG_DEBUG("[RECEIVE] (Receiver) Received expected seqno="
                        << m_expected_seqno);
                m_expected_seqno = (m_expected_seqno + 1) % m_max_seqno;
                m_rxCallback (this, packet, protocol, from);
            }
            else // not an ACK but also not correct seqno, so DROP
            {
                NS_LOG_DEBUG("[RECEIVE] (Receiver) Received unexpected seqno="
                        << header.GetSeqno());
                m_phyRxDropTrace (packet);
            }
        }
    }

    if (!m_promiscCallback.IsNull ())
    {
        m_promiscCallback (this, packet, protocol, from, to, packetType);
    }
}

void 
GbnNetDevice::SetChannel (Ptr<GbnChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);
  m_channel = channel;
  m_channel->Add (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

Ptr<Queue>
GbnNetDevice::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
GbnNetDevice::SetQueue (Ptr<Queue> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
GbnNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void 
GbnNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}
uint32_t 
GbnNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}
Ptr<Channel> 
GbnNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}
void
GbnNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}
Address 
GbnNetDevice::GetAddress (void) const
{
  //
  // Implicit conversion from Mac48Address to Address
  //
  NS_LOG_FUNCTION (this);
  return m_address;
}
bool 
GbnNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}
uint16_t 
GbnNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}
bool 
GbnNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}
void 
GbnNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
 NS_LOG_FUNCTION (this << &callback);
 m_linkChangeCallbacks.ConnectWithoutContext (callback);
}
bool 
GbnNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
Address
GbnNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}
bool 
GbnNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
Address 
GbnNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address GbnNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool 
GbnNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return true;
    }
  return false;
}

bool 
GbnNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool 
GbnNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);

  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
GbnNetDevice::SendFrom (Ptr<Packet> p, const Address& source, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION (this << p << source << dest << protocolNumber);
    if (p->GetSize() > GetMtu()) { return false; }

    Mac48Address to = Mac48Address::ConvertFrom(dest);
    Mac48Address from = Mac48Address::ConvertFrom(source);

    GbnTag tag;
    tag.SetSrc(from);
    tag.SetDst(to);
    tag.SetProto(protocolNumber);

    GbnHeader header;
    header.SetSeqno(m_seqno);
    m_seqno = (m_seqno + 1) % m_max_seqno;

    p->AddPacketTag(tag);
    p->AddHeader(header);

    if (m_queue->Enqueue(p))
    {
        if (!isWindowFull())
        {
            PacketPair packetPair = std::make_pair(p, EventId());

            if (!TransmitCompleteEvent.IsRunning ())
            {
                Time txTime = Time (0);
                if (m_bps > DataRate (0))
                {
                    txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
                }
                NS_ASSERT(txTime < timeoutTime);
                packetPair.second = Simulator::Schedule (timeoutTime, &GbnNetDevice::Timeout, this);
                TransmitCompleteEvent = Simulator::Schedule (txTime, &GbnNetDevice::TransmitComplete, this);
                NS_LOG_DEBUG("[SEND FROM] (Sender) Scheduling packet "
                        << header.GetSeqno() << " for "
                        << Simulator::Now().GetSeconds() + txTime.GetSeconds());
            }

            NS_LOG_DEBUG("[SEND FROM] (Sender) Pushing onto window of size "
                    << m_window.size() << " seqno=" << header.GetSeqno());
            m_window.push_back (packetPair);
            m_queue->Dequeue ();
            NS_LOG_DEBUG("[SEND FROM] (Sender) Dequeued to size "
                    << m_queue->GetNPackets());
            GbnHeader h; m_inflight->first->PeekHeader(h);
            NS_LOG_DEBUG("[SEND FROM] (Sender) m_inflight points to seqno="
                    << h.GetSeqno());
        }

        return true;
    }

    return false;
}

void
GbnNetDevice::Timeout ()
{
    NS_LOG_FUNCTION (this);

    NS_ASSERT(m_window.size() > 0);

    NS_LOG_DEBUG("[TIMEOUT] Shifting m_inflight to beginning of window");
    m_inflight = m_window.begin();
    m_inflight->second.Cancel();

    if (!TransmitCompleteEvent.IsRunning ())
    {
        Time txTime = Time (0);
        if (m_bps > DataRate (0))
        {
            txTime = m_bps.CalculateBytesTxTime (m_inflight->first->GetSize ());
        }

        NS_ASSERT(txTime < timeoutTime);

        m_inflight->second = Simulator::Schedule (timeoutTime, &GbnNetDevice::Timeout, this);
        TransmitCompleteEvent = Simulator::Schedule (txTime, &GbnNetDevice::TransmitComplete, this);
    }
}

void
GbnNetDevice::TransmitComplete ()
{
  NS_LOG_FUNCTION (this);

  if (isWindowEmpty()) {
      NS_LOG_DEBUG("[TRANSMIT COMPLETE] (Sender) Window is empty");
      return;
  }

  // --------------------------------------------------------------------------
  // Transmit finished packet
  GbnTag tag;
  m_inflight->first->PeekPacketTag (tag);
  // NOTE: We cannot remove the packet tag as if this packet is dropped and
  // retransmitted, we need the tags to properly send it to its destination

  Mac48Address src = tag.GetSrc ();
  Mac48Address dst = tag.GetDst ();
  uint16_t proto = tag.GetProto ();

  GbnHeader h; m_inflight->first->PeekHeader(h); // debugging
  NS_LOG_DEBUG("[TRANSMIT COMPLETE] (Sender) Sending packet " << h.GetSeqno()
          << " at " << Simulator::Now().GetSeconds());
  m_channel->Send(m_inflight->first, proto, dst, src, this);
  // --------------------------------------------------------------------------

  // --------------------------------------------------------------------------
  // Transmit next packet in window
  m_inflight++;

  if (isWindowEmpty()) {
      NS_LOG_DEBUG("[TRANSMIT COMPLETE] (Sender) Window is empty");
      return;
  }

  m_inflight->second.Cancel();

  Time txTime = Time (0);
  if (m_bps > DataRate (0))
  {
      txTime = m_bps.CalculateBytesTxTime (m_inflight->first->GetSize ());
  }

  NS_ASSERT(txTime < timeoutTime);

  m_inflight->second = Simulator::Schedule (timeoutTime, &GbnNetDevice::Timeout, this);
  TransmitCompleteEvent = Simulator::Schedule (txTime, &GbnNetDevice::TransmitComplete, this);
  m_inflight->first->PeekHeader(h); // debugging
  NS_LOG_DEBUG("[TRANSMIT COMPLETE] (Sender) Scheduling packet "
          << h.GetSeqno() << " for "
          << Simulator::Now().GetSeconds() + txTime.GetSeconds());
  // --------------------------------------------------------------------------
}

bool
GbnNetDevice::isWindowFull (void) const
{
  NS_LOG_FUNCTION (this);
  return m_wsize == m_window.size();
}

bool
GbnNetDevice::isWindowEmpty (void) const
{
  NS_LOG_FUNCTION (this);
  return m_inflight == m_window.end();
}

Ptr<Node> 
GbnNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}
void 
GbnNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}
bool 
GbnNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
void 
GbnNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void
GbnNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_node = 0;
  m_receiveErrorModel = 0;
  m_queue->DequeueAll ();
  if (TransmitCompleteEvent.IsRunning ())
    {
      TransmitCompleteEvent.Cancel ();
    }
  NetDevice::DoDispose ();
}


void
GbnNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_promiscCallback = cb;
}

bool
GbnNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

} // namespace ns3
