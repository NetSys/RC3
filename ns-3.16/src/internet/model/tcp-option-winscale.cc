/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
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
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#include "tcp-option-winscale.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionWinScale);

TcpOptionWinScale::TcpOptionWinScale ()
  : m_scale (0)
{
}

TcpOptionWinScale::~TcpOptionWinScale ()
{
}

TypeId
TcpOptionWinScale::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionWinScale")
    .SetParent<TcpOption> ()
  ;
  return tid;
}

TypeId
TcpOptionWinScale::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionWinScale::Print (std::ostream &os) const
{
  os << m_scale;
}

uint32_t
TcpOptionWinScale::GetSerializedSize (void) const
{
  return 3;
}

void
TcpOptionWinScale::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (3); // Kind
  i.WriteU8 (3); // Length
  i.WriteU8 (m_scale); // Max segment size
} 

uint32_t
TcpOptionWinScale::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t size = i.ReadU8 ();
  NS_ASSERT (size == 3);
  m_scale = i.ReadU8 ();
  return 3;
}

uint8_t
TcpOptionWinScale::GetKind (void) const
{
  return 3;
}

uint8_t
TcpOptionWinScale::GetScale (void) const
{
  return m_scale;
}

void
TcpOptionWinScale::SetScale (uint8_t scale)
{
  m_scale = scale;
}

} // namespace ns3
