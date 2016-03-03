/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef GBN_NETDEVICE_HELPER_H
#define GBN_NETDEVICE_HELPER_H

#include <string>

#include "ns3/attribute.h"
#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/gbn-channel.h"

namespace ns3 {

/**
 * \brief build a set of GbnNetDevice objects
 */
class GbnNetDeviceHelper
{
public:
  /**
   * Construct a GbbnNetDeviceHelper.
   */
  GbnNetDeviceHelper ();
  virtual ~GbnNetDeviceHelper () {}

  /**
   * Each net device must have a queue to pass packets through.
   * This method allows one to set the type of the queue that is automatically
   * created when the device is created and attached to a node.
   *
   * \param type the type of queue
   * \param n1 the name of the attribute to set on the queue
   * \param v1 the value of the attribute to set on the queue
   * \param n2 the name of the attribute to set on the queue
   * \param v2 the value of the attribute to set on the queue
   * \param n3 the name of the attribute to set on the queue
   * \param v3 the value of the attribute to set on the queue
   * \param n4 the name of the attribute to set on the queue
   * \param v4 the value of the attribute to set on the queue
   *
   * Set the type of queue to create and associated to each
   * GbnNetDevice created through GbbnNetDeviceHelper::Install.
   */
  void SetQueue (std::string type,
                 std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                 std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                 std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                 std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());

  /**
   * Each net device must have a channel to pass packets through.
   * This method allows one to set the type of the channel that is automatically
   * created when the device is created and attached to a node.
   *
   * \param type the type of queue
   * \param n1 the name of the attribute to set on the queue
   * \param v1 the value of the attribute to set on the queue
   * \param n2 the name of the attribute to set on the queue
   * \param v2 the value of the attribute to set on the queue
   * \param n3 the name of the attribute to set on the queue
   * \param v3 the value of the attribute to set on the queue
   * \param n4 the name of the attribute to set on the queue
   * \param v4 the value of the attribute to set on the queue
   *
   * Set the type of channel to create and associated to each
   * GbnNetDevice created through GbbnNetDeviceHelper::Install.
   */
  void SetChannel (std::string type,
                   std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                   std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                   std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                   std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());


  /**
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   *
   * Set these attributes on each ns3::GbnNetDevice created
   * by GbnNetDeviceHelper::Install
   */
  void SetDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   *
   * Set these attributes on each ns3::CsmaChannel created
   * by GbnNetDeviceHelper::Install
   */
  void SetChannelAttribute (std::string n1, const AttributeValue &v1);

  /**
   * GbnNetDevice is Broadcast capable and ARP needing. This function
   * limits the number of GbnNetDevices on one channel to two, disables
   * Broadcast and ARP and enables PointToPoint mode.
   *
   * \warning It must be used before installing a NetDevice on a node.
   *
   * \param pointToPointMode True for PointToPoint GbnNetDevice
   */
  void SetNetDevicePointToPointMode (bool pointToPointMode);

  /**
   * This method creates an ns3::GbnChannel with the attributes configured by
   * GbbnNetDeviceHelper::SetChannelAttribute, an ns3::GbnNetDevice with the attributes
   * configured by GbnNetDeviceHelper::SetDeviceAttribute and then adds the device
   * to the node and attaches the channel to the device.
   *
   * \param node The node to install the device in
   * \returns A container holding the added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node) const;

  /**
   * This method creates an ns3::GbnNetDevice with the attributes configured by
   * GbbnNetDeviceHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param node The node to install the device in
   * \param channel The channel to attach to the device.
   * \returns A container holding the added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node, Ptr<GbnChannel> channel) const;

  /**
   * This method creates an ns3::GbnChannel with the attributes configured by
   * GbbnNetDeviceHelper::SetChannelAttribute.  For each Ptr<node> in the provided
   * container: it creates an ns3::GbnNetDevice (with the attributes
   * configured by GbnNetDeviceHelper::SetDeviceAttribute); adds the device to the
   * node; and attaches the channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed.
   * \returns A container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c) const;

  /**
   * For each Ptr<node> in the provided container, this method creates an
   * ns3::GbnNetDevice (with the attributes configured by
   * GbbnNetDeviceHelper::SetDeviceAttribute); adds the device to the node; and attaches
   * the provided channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed.
   * \param channel The channel to attach to the devices.
   * \returns A container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c, Ptr<GbnChannel> channel) const;

private:
  /**
   * This method creates an ns3::GbnNetDevice with the attributes configured by
   * GbbnNetDeviceHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param node The node to install the device in
   * \param channel The channel to attach to the device.
   * \returns The new net device.
   */
  Ptr<NetDevice> InstallPriv (Ptr<Node> node, Ptr<GbnChannel> channel) const;

  ObjectFactory m_queueFactory; //!< Queue factory
  ObjectFactory m_deviceFactory; //!< NetDevice factory
  ObjectFactory m_channelFactory; //!< Channel factory
  bool m_pointToPointMode; //!< Install PointToPoint GbnNetDevice or Broadcast ones

};

} // namespace ns3

#endif /* GBN_NETDEVICE_HELPER_H */
