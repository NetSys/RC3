/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
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
 * Author: Radhika Mittal, based on ns-2 scoreboard implementation
 */

#ifndef SCOREBOARD
#define SCOREBOARD

#include <stdint.h>
#include <map>
#include "ns3/header.h"
#include "ns3/tcp-option.h"
#include "ns3/buffer.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/sequence-number.h"
#include "ns3/tcp-header.h"
#include "ns3/ptr.h"

namespace ns3 {

struct ScoreBoardNode
{
  bool acked;
  bool sacked;
  bool lowPr;
};

class ScoreBoard
{
  public:

    static TypeId GetTypeId (void);

    ScoreBoard();
    virtual ~ScoreBoard();

    virtual void CreateScoreBoard(int size, uint32_t mss, uint32_t retxThresh);
    virtual void ClearScoreBoard();
    virtual void UpdateScoreBoard(const TcpHeader &tcpHeader);
    virtual bool IsLost(SequenceNumber32 seqNo, SequenceNumber32 nextTxSequence);
    uint32_t SetPipe(SequenceNumber32 headSequence, SequenceNumber32 nextTxSequence);
    virtual SequenceNumber32 GetNextSegment(SequenceNumber32 headSequence, SequenceNumber32 nextTxSequence);
    virtual SequenceNumber32 GetNextAggSegment(SequenceNumber32 nextTxSequence);
    virtual void Print(std::ostream &os, SequenceNumber32 nextTxSequence, SequenceNumber32 headSequence);

    SequenceNumber32 m_highReTx;


  private:
    std::map<SequenceNumber32, ScoreBoardNode > m_sbn;
    uint32_t m_mss; //segment size
    uint32_t m_retxThresh;
};
}
#endif
