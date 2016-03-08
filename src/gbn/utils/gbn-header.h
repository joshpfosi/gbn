/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>

namespace ns3 {

class GbnHeader : public Header 
{
public:

  GbnHeader ();
  virtual ~GbnHeader ();

  void SetSeqno (uint64_t seqno);
  uint64_t GetSeqno (void) const;
  void SetIsAck (bool ack);
  bool GetIsAck (void) const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;
private:
  uint64_t m_seqno;
  uint8_t m_isack;
};

} // namespace ns3
