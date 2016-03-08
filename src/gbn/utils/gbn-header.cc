#include "gbn-header.h"

namespace ns3 {

GbnHeader::GbnHeader () : m_isack(false) {}
GbnHeader::~GbnHeader () {}

NS_OBJECT_ENSURE_REGISTERED (GbnHeader);

TypeId
GbnHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GbnHeader")
    .SetParent<Header> ()
    .AddConstructor<GbnHeader> ()
  ;
  return tid;
}
TypeId
GbnHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
GbnHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "seqno=" << m_seqno << "ack=" << m_isack;
}
uint32_t
GbnHeader::GetSerializedSize (void) const
{
  return sizeof(m_seqno) + sizeof(m_isack);
}
void
GbnHeader::Serialize (Buffer::Iterator start) const
{
  // we can serialize two bytes at the start of the buffer.
  // we write them in network byte order.
  start.WriteHtonU64 (m_seqno);
  start.Write (&m_isack, sizeof(m_isack));
}
uint32_t
GbnHeader::Deserialize (Buffer::Iterator start)
{
  // we can deserialize two bytes from the start of the buffer.
  // we read them in network byte order and store them
  // in host byte order.
  m_seqno = start.ReadNtohU64 ();
  start.Read(&m_isack, sizeof(m_isack));

  // we return the number of bytes effectively read.
  return sizeof(m_seqno) + sizeof(m_isack);
}

void
GbnHeader::SetSeqno (uint64_t seqno)
{
  m_seqno = seqno;
}

uint64_t
GbnHeader::GetSeqno (void) const
{
  return m_seqno;
}

void
GbnHeader::SetIsAck (bool ack)
{
  m_isack = ack;
}

bool
GbnHeader::GetIsAck (void) const
{
  return m_isack;
}

} // namespace ns3
