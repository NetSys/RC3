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

#include "tcp-option.h"
#include "tcp-option-end.h"
#include "tcp-option-nop.h"
#include "tcp-option-mss.h"
#include "tcp-option-winscale.h"
#include "tcp-option-sack-permitted.h"
#include "tcp-option-sack.h"
#include "tcp-option-ts.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOption);

TcpOption::TcpOption ()
{
}

TcpOption::~TcpOption ()
{
}

TypeId
TcpOption::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOption")
    .SetParent<Object> ()
  ;
  return tid;
}

TypeId
TcpOption::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

Ptr<TcpOption>
TcpOption::CreateOption (uint8_t kind)
{
  if (kind == 0)
    {
      return CreateObject<TcpOptionEnd> ();
    }
  else if (kind == 1)
    {
      return CreateObject<TcpOptionNOP> ();
    }
  else if (kind == 2)
    {
      return CreateObject<TcpOptionMSS> ();
    }
  else if (kind == 3)
    {
      return CreateObject<TcpOptionWinScale> ();
    }
  else if (kind == 4)
    {
      return CreateObject<TcpOptionSackPermitted> ();
    }
  else if (kind == 5)
    {
      return CreateObject<TcpOptionSack> ();
    }
  else if (kind == 8)
    {
      return CreateObject<TcpOptionTS> ();
    }
  return NULL;
} 

} // namespace ns3
