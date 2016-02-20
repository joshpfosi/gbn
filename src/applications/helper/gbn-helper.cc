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
#include "gbn-helper.h"
#include "ns3/gbn-receiver.h"
#include "ns3/gbn-sender.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

GbnReceiverHelper::GbnReceiverHelper ()
{
  m_factory.SetTypeId (GbnReceiver::GetTypeId ());
}

void 
GbnReceiverHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
GbnReceiverHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
GbnReceiverHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
GbnReceiverHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
GbnReceiverHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<GbnReceiver> ();
  node->AddApplication (app);

  return app;
}

GbnSenderHelper::GbnSenderHelper (Address rcvrAddress)
{
  m_factory.SetTypeId (GbnSender::GetTypeId ());
  SetAttribute ("RcvrMacAddress", AddressValue (rcvrAddress));
}

void 
GbnSenderHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
GbnSenderHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<GbnSender>()->SetFill (fill);
}

void
GbnSenderHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<GbnSender>()->SetFill (fill, dataLength);
}

void
GbnSenderHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<GbnSender>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
GbnSenderHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
GbnSenderHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
GbnSenderHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
GbnSenderHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<GbnSender> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
