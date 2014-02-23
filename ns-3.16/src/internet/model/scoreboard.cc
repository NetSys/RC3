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

#include <stdint.h>
#include <iostream>
#include "tcp-header.h"
#include "scoreboard.h"
#include "ns3/buffer.h"
#include "tcp-option-sack.h"

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (ScoreBoard);


TypeId 
ScoreBoard::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::ScoreBoard")
          .SetParent<Object> ()
              ;
                return tid;
}


ScoreBoard::ScoreBoard(void):
  m_highReTx(0),
  m_mss(1460),
  m_retxThresh(3)
{
}



ScoreBoard::~ScoreBoard()
{
}


void
ScoreBoard::CreateScoreBoard(int size, uint32_t mss, uint32_t retxThresh)
{
  m_mss = mss;
  m_retxThresh = retxThresh;
  for (int i=1 ; i<size;)
  {
      SequenceNumber32 seq(i);
      m_sbn[seq].acked = false;
      m_sbn[seq].sacked = false;
      i = i + mss;
  }
}

void
ScoreBoard::ClearScoreBoard()
{
  SequenceNumber32 seqNo = SequenceNumber32(1);
  while (m_sbn.find(seqNo) != m_sbn.end())
  {
      m_sbn[seqNo].acked = false;
      m_sbn[seqNo].sacked = false;
      seqNo += m_mss; 
  }
}


void
ScoreBoard::UpdateScoreBoard(const TcpHeader &tcpHeader)
{
    if (0 == (tcpHeader.GetFlags () & TcpHeader::ACK))
      return;

    SequenceNumber32 cumAck = tcpHeader.GetAckNumber() - m_mss;

    //marking data as acked
    while((m_sbn.find(cumAck) != m_sbn.end()) && (m_sbn[cumAck].acked == false))
    {
       m_sbn[cumAck].acked = true;
       cumAck -= m_mss;
    }

    //marking SACKed data
     Ptr<TcpOptionSack> sack_option;
     sack_option = DynamicCast<TcpOptionSack>(tcpHeader.GetOption(5));
     if(sack_option == NULL)
       return;
     uint32_t sackblock_Count = sack_option->SackCount();
     for(int i = 0; i < (int)sackblock_Count; i++)
     {
       SackBlock s = sack_option->GetSack(i);
       SequenceNumber32 seqNo = s.first;
       while(seqNo < s.second)
       {
          m_sbn[seqNo].sacked = true;
          seqNo += m_mss; 
       }
     }

}
 
bool
ScoreBoard::IsLost(SequenceNumber32 seqNo, SequenceNumber32 nextTxSequence)
{
  NS_ASSERT(m_sbn.find(seqNo) != m_sbn.end());
  if(m_sbn.find(seqNo) == m_sbn.end())
    return false;
  if((m_sbn[seqNo].acked) || (m_sbn[seqNo].sacked))
    return false;

  int count = 0;
  SequenceNumber32 temp = seqNo + m_mss;
  while((temp < nextTxSequence) && (m_sbn.find(temp) != m_sbn.end()))
  {
      NS_ASSERT(!m_sbn[temp].acked);
      if(m_sbn[temp].sacked)
        count++;
      if(count == (int)m_retxThresh)
        return true;
      temp += m_mss;
  }
  return false;

}

uint32_t
ScoreBoard::SetPipe(SequenceNumber32 headSequence, SequenceNumber32 nextTxSequence)
{
  uint32_t pipe = 0;
  SequenceNumber32 seqNo = headSequence;
  while((seqNo < nextTxSequence) && (m_sbn.find(seqNo) != m_sbn.end()))
  {
    if(!m_sbn[seqNo].sacked)
    {
        if(!IsLost(seqNo, nextTxSequence))
            pipe++;
        if(seqNo <= m_highReTx)
            pipe++;
    }
    seqNo += m_mss;
  }
  return (pipe*m_mss);
}

SequenceNumber32
ScoreBoard::GetNextSegment(SequenceNumber32 headSequence, SequenceNumber32 nextTxSequence)
{
  
  SequenceNumber32 seqNo = headSequence;
  while((seqNo < nextTxSequence) && (m_sbn.find(seqNo) != m_sbn.end()))
  {
    if((seqNo > m_highReTx) && (IsLost(seqNo, nextTxSequence)))
      return seqNo;
    seqNo += m_mss;
  }

  //added for double tcp case, otherwise just return m_nextTxSequence
  seqNo = nextTxSequence;
  while(m_sbn.find(seqNo) != m_sbn.end())
  {
    if((!m_sbn[seqNo].sacked) && (!m_sbn[seqNo].acked))
      return seqNo;
    seqNo += m_mss;
  }
  return SequenceNumber32(0);
}

//For double tcp
SequenceNumber32
ScoreBoard::GetNextAggSegment(SequenceNumber32 nextTxSequence)
{
 
  //added for double tcp case, otherwise just return m_nextTxSequence
  SequenceNumber32 seqNo = nextTxSequence;
  while(m_sbn.find(seqNo) != m_sbn.end())
  {
    if((!m_sbn[seqNo].sacked) && (!m_sbn[seqNo].acked))
      return seqNo;
    seqNo += m_mss;
  }
  return SequenceNumber32(0);
}




void 
ScoreBoard::Print(std::ostream &os, SequenceNumber32 nextTxSequence, SequenceNumber32 headSequence)
{
    os<<"Size of scoreboard:"<<m_sbn.size()<<"\n";
    os<<"NextTx: "<<nextTxSequence<<"\t Head Sequence: "<<headSequence<<"\n";
    for(std::map<SequenceNumber32,ScoreBoardNode>::iterator it=m_sbn.begin(); it!=m_sbn.end(); ++it)
    {
      os<<it->first<<"\t Acked: "<<(it->second).acked<<"\t Sacked: "<<(it->second).sacked<<"\t Lost: "<<IsLost(it->first,nextTxSequence)<<"\n"; 
    }
    os<<"Pipe: "<<SetPipe(headSequence, nextTxSequence)<<"\n";
    os<<"High ReTx: "<<m_highReTx<<"\n";
    os<<"NextSegment: "<<GetNextAggSegment(nextTxSequence)<<"\n";
}


}
