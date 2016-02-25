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

#ifndef GBN_RECEIVER_H
#define GBN_RECEIVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/mac48-address.h"
#include "ns3/simple-net-device.h"

namespace ns3 {

class Packet;

class GbnReceiver : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  GbnReceiver ();
  virtual ~GbnReceiver ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  bool HandleRead (Ptr<NetDevice> dev, Ptr<const Packet> p,
          uint16_t protocol, const Address &mac);

  Ptr<NetDevice> m_dev;

  uint64_t m_bytes_rx;
  double m_last_rx;
};

} // namespace ns3

#endif /* GBN_RECEIVER_H */

