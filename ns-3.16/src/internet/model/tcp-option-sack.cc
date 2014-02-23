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

#include "tcp-option-sack.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionSack);

TcpOptionSack::TcpOptionSack ()
: m_kind(5)
{
}

TcpOptionSack::~TcpOptionSack ()
{
}

TypeId
TcpOptionSack::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionSack")
    .SetParent<TcpOption> ()
  ;
  return tid;
}

TypeId
TcpOptionSack::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionSack::Print (std::ostream &os) const
{
  os<<"**************";
  os<<"Size of Sack Option Score Board"<<m_sb.size()<<"\n";
  for (ScoreBoardEntry::const_iterator it = m_sb.begin (); it != m_sb.end (); ++it)
  {
    os<<"Left: "<<it->first<<"\t Right: "<<it->second<<"\n";
  }
  os<<"***********";
}

uint32_t
TcpOptionSack::GetSerializedSize (void) const
{
  return (2+8*m_sb.size());
}

void
TcpOptionSack::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (5); // Kind
  i.WriteU8 (2+8*m_sb.size()); // Length
  for (ScoreBoardEntry::const_iterator it = m_sb.begin (); it != m_sb.end (); ++it)
  {
      i.WriteHtonU32((it->first).GetValue());
      i.WriteHtonU32((it->second).GetValue());
  }
} 

uint32_t
TcpOptionSack::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  //m_kind = i.ReadU8 ();
  //NS_ASSERT(m_kind == 5);
  uint8_t size = i.ReadU8 ();
  uint32_t sb_size = (size - 2)/8;
  for(int it = 0; it<(int)sb_size; it++)
  {
    SequenceNumber32 left = SequenceNumber32(i.ReadNtohU32());
    SequenceNumber32 right = SequenceNumber32(i.ReadNtohU32());
    SackBlock s = std::make_pair(left, right);
    m_sb.push_back(s);
  }
  return size;
}

uint8_t
TcpOptionSack::GetKind (void) const
{
  return 5;
}

void
TcpOptionSack::AddSack (SackBlock s)
{
  // Assumed s has no overlap with any SACK block in m_sb
  m_sb.push_back(s);
  /*for (ScoreBoardEntry::iterator i = m_sb.begin (); i != m_sb.end (); ++i)
    {
      if (s.first < i->first)
        {
          m_sb.insert(i,s);
          break;
        }
    }*/
}

uint32_t
TcpOptionSack::SackCount (void) const
{
  return m_sb.size ();
}

void
TcpOptionSack::ClearSack (void)
{
  m_sb.clear ();
}

SackBlock
TcpOptionSack::GetSack (int offset)
{
  ScoreBoardEntry::iterator i = m_sb.begin ();
  while (offset--) ++i;
  return *i;
}

} // namespace ns3
