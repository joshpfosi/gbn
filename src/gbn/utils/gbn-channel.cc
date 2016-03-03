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
#include <algorithm>
#include "gbn-channel.h"
#include "gbn-net-device.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GbnChannel");

NS_OBJECT_ENSURE_REGISTERED (GbnChannel);

TypeId 
GbnChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GbnChannel")
    .SetParent<Channel> ()
    .SetGroupName("Network")
    .AddConstructor<GbnChannel> ()
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&GbnChannel::m_delay),
                   MakeTimeChecker ())
  ;
  return tid;
}

GbnChannel::GbnChannel ()
{
  NS_LOG_FUNCTION (this);
}

void
GbnChannel::Send (Ptr<Packet> p, uint16_t protocol,
                     Mac48Address to, Mac48Address from,
                     Ptr<GbnNetDevice> sender)
{
  NS_LOG_FUNCTION (this << p << protocol << to << from << sender);
  for (std::vector<Ptr<GbnNetDevice> >::const_iterator i = m_devices.begin (); i != m_devices.end (); ++i)
    {
      Ptr<GbnNetDevice> tmp = *i;
      if (tmp == sender)
        {
          continue;
        }
      if (m_blackListedDevices.find (tmp) != m_blackListedDevices.end ())
        {
          if (find (m_blackListedDevices[tmp].begin (), m_blackListedDevices[tmp].end (), sender) !=
              m_blackListedDevices[tmp].end () )
            {
              continue;
            }
        }
      Simulator::ScheduleWithContext (tmp->GetNode ()->GetId (), m_delay,
                                      &GbnNetDevice::Receive, tmp, p->Copy (), protocol, to, from);
    }
}

void
GbnChannel::Add (Ptr<GbnNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  m_devices.push_back (device);
}

uint32_t
GbnChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_devices.size ();
}

Ptr<NetDevice>
GbnChannel::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION (this << i);
  return m_devices[i];
}

void
GbnChannel::BlackList (Ptr<GbnNetDevice> from, Ptr<GbnNetDevice> to)
{
  if (m_blackListedDevices.find (to) != m_blackListedDevices.end ())
    {
      if (find (m_blackListedDevices[to].begin (), m_blackListedDevices[to].end (), from) ==
          m_blackListedDevices[to].end () )
        {
          m_blackListedDevices[to].push_back (from);
        }
    }
  else
    {
      m_blackListedDevices[to].push_back (from);
    }
}

void
GbnChannel::UnBlackList (Ptr<GbnNetDevice> from, Ptr<GbnNetDevice> to)
{
  if (m_blackListedDevices.find (to) != m_blackListedDevices.end ())
    {
      std::vector<Ptr<GbnNetDevice> >::iterator iter;
      iter = find (m_blackListedDevices[to].begin (), m_blackListedDevices[to].end (), from);
      if (iter != m_blackListedDevices[to].end () )
        {
          m_blackListedDevices[to].erase (iter);
        }
    }
}


} // namespace ns3
