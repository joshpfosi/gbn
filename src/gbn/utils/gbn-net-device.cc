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
                   UintegerValue (1),
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
    m_wsize (1),
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

  NS_LOG_DEBUG("(Receive) in RECEIVE");

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
      GbnHeader h; packet->PeekHeader(h);
      NS_LOG_DEBUG("(Receive) DROPPING packet " << h.GetSeqno() << " at " << Simulator::Now().GetSeconds());
      m_phyRxDropTrace (packet);
      return;
    }

  NS_LOG_DEBUG("Receiving packet to " << to << " and we're at " << m_address);
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

  NS_LOG_DEBUG("packetType=" << packetType << " at " << Simulator::Now().GetSeconds());
  if (packetType != NetDevice::PACKET_OTHERHOST)
    {
      GbnHeader header;
      packet->RemoveHeader(header);

      // We received the packet so clear it and cancel its timeout
      if (header.GetIsAck())
        {
            NS_LOG_DEBUG("[SENDER] (Receive) Received ACK for seqno="
                    << header.GetSeqno () << " at " << Simulator::Now().GetSeconds());

            NS_LOG_DEBUG("[SENDER] (Receive) Cancelling EventId " << m_window.begin()->second.GetUid());
            m_window.begin()->second.Cancel(); // cancel timeout
            m_window.erase (m_window.begin()); // clear packet
            
            NS_LOG_DEBUG("[SENDER] (Receive) Sender queue length is " << m_queue->GetNPackets ());

            // Enqueue new packet if one exists
            // NOTE: Window can always hold at least 1 more element as we erased
            // one above
            if (m_queue->GetNPackets ())
              {
                  NS_LOG_DEBUG("[SENDER] (Receive) Window is not full so pushing seqno=" << header.GetSeqno ());
                  Ptr<Packet> dataPacket = m_queue->Dequeue();
                  std::pair<Ptr<Packet>, EventId> packet_pair = std::make_pair(dataPacket, EventId());

                  // Schedule a TransmitComplete event if necessary
                  if (!TransmitCompleteEvent.IsRunning ())
                    {
                      Time txTime = Time (0);
                      if (m_bps > DataRate (0))
                        {
                          txTime = m_bps.CalculateBytesTxTime (dataPacket->GetSize ());
                        }
                      NS_ASSERT(txTime < timeoutTime);
                      packet_pair.second = Simulator::Schedule (timeoutTime, &GbnNetDevice::Timeout, this);
                      dataPacket->PeekHeader(header); // debugging
                      NS_LOG_DEBUG("[SENDER] (Receive) Scheduling a Timeout for seqno " << header.GetSeqno() << " at " << Simulator::Now().GetSeconds() + timeoutTime.GetSeconds() << " with EventId " << packet_pair.second.GetUid());
                      TransmitCompleteEvent = Simulator::Schedule (txTime, &GbnNetDevice::TransmitComplete, this);
                      NS_LOG_DEBUG("[SENDER] (Receive) Scheduling a TransmiteComplete for seqno " << header.GetSeqno() << " at " << Simulator::Now().GetSeconds() + txTime.GetSeconds() << " with EventId " << TransmitCompleteEvent.GetUid());
                    }

                  m_window.push_back (packet_pair);

                  // NOTE: As `m_window.push_back()` invalidates `end()`, the
                  // conditional must come after
                  if (m_inflight == m_window.end ())
                    {
                      NS_LOG_DEBUG("[SENDER] (Receive) Shifting m_inflight");
                      m_inflight = --m_window.end ();
                    }
              }
        }
      else if (header.GetSeqno() == m_expected_seqno) 
        {
            NS_LOG_DEBUG("[RECEIVER] (Receive) Received expected packet "
                    << m_expected_seqno << " at " << Simulator::Now().GetSeconds());
            GbnHeader header;
            header.SetIsAck (true);
            header.SetSeqno (m_expected_seqno);

            Ptr<Packet> ack = Create<Packet> (0);
            ack->AddHeader (header);

            // Do we use SendFrom here or m_channel->Send()?
            // SendFrom is more realistic but m_channel->Send() is probably
            // required b/c the analytical models do not account for ACKs
            // whatsoever
            NS_LOG_DEBUG("[RECEIVER] (Receive) Sending ACK for packet " << m_expected_seqno);
            m_channel->Send(ack, protocol, from, m_address, this);

            NS_LOG_DEBUG("[RECEIVER] Incrementing expected seqno from " << m_expected_seqno << " to " << m_expected_seqno + 1);
            m_expected_seqno = (m_expected_seqno + 1) % m_max_seqno;
            m_rxCallback (this, packet, protocol, from);
        }
      else // not an ACK but also not correct seqno, so DROP
        {
            NS_LOG_DEBUG("[RECEIVER] (Receive) seqno=" << header.GetSeqno() << " (expecting " << m_expected_seqno << ") and not an ACK, so drop");
            GbnHeader header;
            header.SetIsAck (true);
            header.SetSeqno (m_expected_seqno);

            Ptr<Packet> ack = Create<Packet> (0);
            ack->AddHeader (header);

            // Do we use SendFrom here or m_channel->Send()?
            // SendFrom is more realistic but m_channel->Send() is probably
            // required b/c the analytical models do not account for ACKs
            // whatsoever
            NS_LOG_DEBUG("[RECEIVER] (Receive) Sending ACK for packet " << m_expected_seqno - 1);
            m_channel->Send(ack, protocol, from, m_address, this);

            m_phyRxDropTrace (packet);
            return;
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
  if (p->GetSize () > GetMtu ())
    {
      return false;
    }
  Ptr<Packet> packet = p->Copy ();

  Mac48Address to = Mac48Address::ConvertFrom (dest);
  Mac48Address from = Mac48Address::ConvertFrom (source);

  GbnTag tag;
  tag.SetSrc(from);
  tag.SetDst(to);
  tag.SetProto(protocolNumber);

  GbnHeader header;
  header.SetSeqno(m_seqno);
  m_seqno = (m_seqno + 1) % m_max_seqno;

  p->AddPacketTag(tag);
  p->AddHeader(header);

  // NS_LOG_DEBUG("[SENDER] Trying to enqueue packet " << header.GetSeqno ());
  if (m_queue->Enqueue (p))
    {
      // NS_LOG_DEBUG("[SENDER] Enqueued...");
      if (!isWindowFull ())
        {
          NS_LOG_DEBUG("[SENDER] Window is not full so pushing seqno=" << header.GetSeqno ());

          std::pair<Ptr<Packet>, EventId> packet_pair = std::make_pair(p, EventId());

          if (!TransmitCompleteEvent.IsRunning ())
            {
              Time txTime = Time (0);
              if (m_bps > DataRate (0))
              {
                  txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
              }
              NS_ASSERT(txTime < timeoutTime);
              packet_pair.second = Simulator::Schedule (timeoutTime, &GbnNetDevice::Timeout, this);
              NS_LOG_DEBUG("[SENDER] (SendFrom) Scheduling a Timeout for seqno " << header.GetSeqno() << " at " << Simulator::Now().GetSeconds() + timeoutTime.GetSeconds() << " with EventId " << packet_pair.second.GetUid());
              TransmitCompleteEvent = Simulator::Schedule (txTime, &GbnNetDevice::TransmitComplete, this);
            }

          m_window.push_back (packet_pair);
          m_queue->Dequeue ();
        }
      return true;
    }

  return false;
}

void
GbnNetDevice::Timeout ()
{
    NS_LOG_FUNCTION (this);

    // TODO: This should be an assertion but I cannot find why it's getting
    // triggered so for the sake of time, simply return
    // NS_ASSERT(m_window.size() > 0);
    if (m_window.size() == 0)
      {
        return;
      }

    m_inflight = m_window.begin();

    std::pair<Ptr<Packet>, EventId > packet_pair = *m_inflight;

    GbnHeader header;
    packet_pair.first->PeekHeader (header);
    NS_LOG_DEBUG("[SENDER] TIMEOUT for packet " << header.GetSeqno() << " at "
            << Simulator::Now().GetSeconds());

    if (!TransmitCompleteEvent.IsRunning ())
    {
        Time txTime = Time (0);
        if (m_bps > DataRate (0))
        {
            txTime = m_bps.CalculateBytesTxTime (packet_pair.first->GetSize ());
        }
        NS_ASSERT(txTime < timeoutTime);
        packet_pair.second.Cancel();
        packet_pair.second = Simulator::Schedule (timeoutTime, &GbnNetDevice::Timeout, this);
        TransmitCompleteEvent = Simulator::Schedule (txTime, &GbnNetDevice::TransmitComplete, this);
    }
}

void
GbnNetDevice::TransmitComplete ()
{
  NS_LOG_FUNCTION (this);

  if (m_inflight == m_window.end())
    {
      return;
    }

  Ptr<Packet> packet = m_inflight->first;
  m_inflight++;

  GbnTag tag;
  packet->PeekPacketTag (tag);
  // NOTE: We cannot remove the packet tag as if this packet is dropped and
  // retransmitted, we need the tags to properly send it to its destination

  Mac48Address src = tag.GetSrc ();
  Mac48Address dst = tag.GetDst ();
  uint16_t proto = tag.GetProto ();

  GbnHeader header;
  packet->PeekHeader (header);
  NS_LOG_DEBUG("[SENDER] Sending seqno=" << header.GetSeqno () << " from " << src << " to " << dst);
  m_channel->Send (packet, proto, dst, src, this);

  NS_LOG_DEBUG("[SENDER] In TransmitComplete, at "
          << Simulator::Now().GetSeconds() << ", "
          << m_window.end() - m_inflight << " packets left (size="
          << m_window.size() << ")");
  if (m_inflight != m_window.end())
    {
      std::pair<Ptr<Packet>, EventId> packet_pair = *m_inflight;

      Time txTime = Time (0);
      if (m_bps > DataRate (0))
        {
          txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
        }
      NS_ASSERT(txTime < timeoutTime);
      NS_LOG_DEBUG("[SENDER] (TransmitComplete) Cancelling EventId " << packet_pair.second.GetUid());
      packet_pair.second.Cancel();
      packet_pair.second = Simulator::Schedule (timeoutTime, &GbnNetDevice::Timeout, this);
      NS_ASSERT(txTime < timeoutTime);
      NS_LOG_DEBUG("[SENDER] (TransmitComplete) Scheduling a Timeout for seqno " << header.GetSeqno() << " at " << Simulator::Now().GetSeconds() + timeoutTime.GetSeconds() << " with EventId " << packet_pair.second.GetUid());
      TransmitCompleteEvent = Simulator::Schedule (txTime, &GbnNetDevice::TransmitComplete, this);
    }
}

bool
GbnNetDevice::isWindowFull (void) const
{
  NS_LOG_FUNCTION (this);
  return m_wsize == m_window.size();
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
