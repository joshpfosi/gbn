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
#ifndef GBN_CHANNEL_H
#define GBN_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/nstime.h"
#include "ns3/mac48-address.h"
#include <vector>
#include <map>

namespace ns3 {

class GbnNetDevice;
class Packet;

/**
 * \ingroup channel
 * \brief A simple channel, for simple things and testing.
 *
 * This channel doesn't check for packet collisions and it
 * does not introduce any error.
 * By default, it does not add any delay to the packets.
 * Furthermore, it assumes that the associated NetDevices
 * are using 48-bit MAC addresses.
 *
 * This channel is meant to be used by ns3::GbnNetDevices.
 */
class GbnChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  GbnChannel ();

  /**
   * A packet is sent by a net device.  A receive event will be 
   * scheduled for all net device connected to the channel other 
   * than the net device who sent the packet
   *
   * \param p packet to be sent
   * \param protocol protocol number
   * \param to address to send packet to
   * \param from address the packet is coming from
   * \param sender netdevice who sent the packet
   *
   */
  virtual void Send (Ptr<Packet> p, uint16_t protocol, Mac48Address to, Mac48Address from,
                     Ptr<GbnNetDevice> sender);

  /**
   * Attached a net device to the channel.
   *
   * \param device the device to attach to the channel
   */ 
  virtual void Add (Ptr<GbnNetDevice> device);

  /**
   * Blocks the communications from a NetDevice to another NetDevice.
   * The block is unidirectional
   *
   * \param from the device to BlackList
   * \param to the device wanting to block the other one
   */
  virtual void BlackList (Ptr<GbnNetDevice> from, Ptr<GbnNetDevice> to);

  /**
   * Un-Blocks the communications from a NetDevice to another NetDevice.
   * The block is unidirectional
   *
   * \param from the device to BlackList
   * \param to the device wanting to block the other one
   */
  virtual void UnBlackList (Ptr<GbnNetDevice> from, Ptr<GbnNetDevice> to);

  // inherited from ns3::Channel
  virtual uint32_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;

private:
  Time m_delay; //!< The assigned speed-of-light delay of the channel
  std::vector<Ptr<GbnNetDevice> > m_devices; //!< devices connected by the channel
  std::map<Ptr<GbnNetDevice>, std::vector<Ptr<GbnNetDevice> > > m_blackListedDevices; //!< devices blocked on a device
};

} // namespace ns3

#endif /* GBN_CHANNEL_H */