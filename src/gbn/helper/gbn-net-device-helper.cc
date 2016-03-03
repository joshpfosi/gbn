/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/object-factory.h"
#include "ns3/gbn-net-device.h"
#include "ns3/gbn-channel.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/boolean.h"

#include "ns3/trace-helper.h"
#include "gbn-net-device-helper.h"

#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GbnNetDeviceHelper");

GbnNetDeviceHelper::GbnNetDeviceHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue");
  m_deviceFactory.SetTypeId ("ns3::GbnNetDevice");
  m_channelFactory.SetTypeId ("ns3::GbnChannel");
  m_pointToPointMode = false;
}

void 
GbnNetDeviceHelper::SetQueue (std::string type,
                                 std::string n1, const AttributeValue &v1,
                                 std::string n2, const AttributeValue &v2,
                                 std::string n3, const AttributeValue &v3,
                                 std::string n4, const AttributeValue &v4)
{
  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void
GbnNetDeviceHelper::SetChannel (std::string type,
                                   std::string n1, const AttributeValue &v1,
                                   std::string n2, const AttributeValue &v2,
                                   std::string n3, const AttributeValue &v3,
                                   std::string n4, const AttributeValue &v4)
{
  m_channelFactory.SetTypeId (type);
  m_channelFactory.Set (n1, v1);
  m_channelFactory.Set (n2, v2);
  m_channelFactory.Set (n3, v3);
  m_channelFactory.Set (n4, v4);
}

void
GbnNetDeviceHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void
GbnNetDeviceHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
}

void
GbnNetDeviceHelper::SetNetDevicePointToPointMode (bool pointToPointMode)
{
  m_pointToPointMode = pointToPointMode;
}

NetDeviceContainer
GbnNetDeviceHelper::Install (Ptr<Node> node) const
{
  Ptr<GbnChannel> channel = m_channelFactory.Create<GbnChannel> ();
  return Install (node, channel);
}

NetDeviceContainer
GbnNetDeviceHelper::Install (Ptr<Node> node, Ptr<GbnChannel> channel) const
{
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer 
GbnNetDeviceHelper::Install (const NodeContainer &c) const
{
  Ptr<GbnChannel> channel = m_channelFactory.Create<GbnChannel> ();

  return Install (c, channel);
}

NetDeviceContainer 
GbnNetDeviceHelper::Install (const NodeContainer &c, Ptr<GbnChannel> channel) const
{
  NetDeviceContainer devs;

  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      devs.Add (InstallPriv (*i, channel));
    }

  return devs;
}

Ptr<NetDevice>
GbnNetDeviceHelper::InstallPriv (Ptr<Node> node, Ptr<GbnChannel> channel) const
{
  Ptr<GbnNetDevice> device = m_deviceFactory.Create<GbnNetDevice> ();
  device->SetAttribute ("PointToPointMode", BooleanValue (m_pointToPointMode));
  device->SetAddress (Mac48Address::Allocate ());
  node->AddDevice (device);
  device->SetChannel (channel);
  Ptr<Queue> queue = m_queueFactory.Create<Queue> ();
  device->SetQueue (queue);
  NS_ASSERT_MSG (!m_pointToPointMode || (channel->GetNDevices () <= 2), "Device set to PointToPoint and more than 2 devices on the channel.");
  return device;
}

} // namespace ns3