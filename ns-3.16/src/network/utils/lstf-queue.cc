/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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

#include <iostream>
#include <fstream>
#include "ns3/simulator.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/my-priority-tag.h"
#include "ns3/random-variable-stream.h"
#include "lstf-queue.h"


#define MAXPACKETS 1000
#define MAXBYTES 225000


NS_LOG_COMPONENT_DEFINE ("LstfQueue");

namespace ns3 {

//priority queue

NS_OBJECT_ENSURE_REGISTERED (LstfQueue);

TypeId LstfQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::LstfQueue")
    .SetParent<Queue> ()
    .AddConstructor<LstfQueue> ()
    .AddAttribute ("Mode", 
                   "Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue size metric.",
                   EnumValue (QUEUE_MODE_BYTES),
                   MakeEnumAccessor (&LstfQueue::SetMode),
                   MakeEnumChecker (QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets accepted by this LstfQueue.",
                   UintegerValue (MAXPACKETS),
                   MakeUintegerAccessor (&LstfQueue::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this LstfQueue.",
                   UintegerValue (MAXBYTES),
                   MakeUintegerAccessor (&LstfQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Id", "The id (unique integer) of this Queue.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&LstfQueue::m_id),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BackgroundDrop", "Background Drop rate",
                    DoubleValue(0.0),
                    MakeDoubleAccessor(&LstfQueue::m_backgrounddrop),
                    MakeDoubleChecker<double> ())  
    ;
  return tid;
}

void
LstfQueue::LogQueueLength()
{
  std::ofstream ofs("queuelength.txt", std::ios::app);
  ofs<<Simulator::Now().GetSeconds()<<"\t"<<m_id<<"\t"<<m_packets.size()<<"\t"<<m_totalpackets<<"\t"<<m_bytesInQueue<<"\t"<<m_bytesInQueue<<"\n";
  ofs.close();
  m_sendEvent = Simulator::Schedule(Seconds(0.1), &LstfQueue::LogQueueLength, this);
}

LstfQueue::LstfQueue () :
  Queue (),
  m_totalpackets (0),
  m_bytesInQueue (0),
  m_id(0),
  m_backgrounddrop(0),
  m_sendEvent()
  //m_time (0),
  //m_interval(1.0)
  //m_packetInfocount(0),
  //m_isPacketInfoFull(false)
{
  NS_LOG_FUNCTION_NOARGS ();
  LogQueueLength();
}

LstfQueue::~LstfQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
LstfQueue::SetMode (LstfQueue::QueueMode mode)
{
  NS_LOG_FUNCTION (mode);
  m_mode = mode;
}

LstfQueue::QueueMode
LstfQueue::GetMode (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mode;
}


uint64_t 
LstfQueue::GetPriorityFromPacket(Ptr <const Packet> p)
{
    MyPriorityTag tag;
    Packet::EnablePrinting();
    bool peeked = p->PeekPacketTag(tag);
    if(!peeked)
    {
      //some rare flagged packets sent directly from l4 might not contain the tag...in that case, just assume 0 priority
      return 0;
    }
    NS_ASSERT(tag.GetTypeId().GetName() == "ns3::MyPriorityTag");
    return (uint64_t)(tag.GetPriority());
}

void
LstfQueue::SetTimestampForPacket(Ptr <Packet> p)
{
    MyPriorityTag tag;
    Packet::EnablePrinting();
    p->RemovePacketTag(tag);
    NS_ASSERT(tag.GetTypeId().GetName() == "ns3::MyPriorityTag");
    //tag.SetTimestamp(Simulator::Now().GetNanoSeconds());
    tag.SetTimestamp(Simulator::Now().GetNanoSeconds());
    p->AddPacketTag(tag);
}

uint64_t
LstfQueue::GetTimestampFromPacket(Ptr <const Packet> p)
{
    MyPriorityTag tag;
    Packet::EnablePrinting();
    bool peeked = p->PeekPacketTag(tag);
    if(!peeked)
    {
      //some rare flagged packets sent directly from l4 might not contain the tag...in that case, just assume 0 priority
      return 0;
    }
    NS_ASSERT(tag.GetTypeId().GetName() == "ns3::MyPriorityTag");
    return tag.GetTimestamp();
}



uint32_t 
LstfQueue::GetIdFromPacket(Ptr <const Packet> p)
{
    MyPriorityTag tag;
    Packet::EnablePrinting();
    bool peeked = p->PeekPacketTag(tag);
    if(!peeked)
    {
      //some rare flagged packets sent directly from l4 might not contain the tag...in that case, just assume 0 priority
      return 0;
    }
    NS_ASSERT(tag.GetTypeId().GetName() == "ns3::MyPriorityTag");
    return tag.GetId();
}

void
LstfQueue::InsertPacketInSortedQueue(Ptr <Packet> p, uint64_t pr)
{
  if (m_packets.empty()) {
    m_packets.push_back(p);
    return;
  }
  Ptr<Packet> tail_p = m_packets.back();
  uint64_t temppr = GetPriorityFromPacket(tail_p);
  if(temppr <= pr) {
    m_packets.push_back(p);
    return;
  }
  for (std::deque<Ptr<Packet> >::iterator it = m_packets.begin(); it != m_packets.end(); it++) {
    temppr = GetPriorityFromPacket(*it);
    if(temppr > pr) {
      m_packets.insert(it, p);
      return;
    }
  }  
  m_packets.push_back(p);
}


bool 
LstfQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p << m_id);
  bool dropped = true;
  uint64_t pr;
  Ptr<Packet> tail_p; 
  uint64_t tail_p_pr;

  SetTimestampForPacket(p);
  pr = GetPriorityFromPacket(p);
  NS_LOG_LOGIC("Enqueueing in priority queue");
  //p->Print(std::cout);
  //std::cout<<std::endl;

  if (m_mode == QUEUE_MODE_PACKETS && (m_totalpackets >= m_maxPackets))
  {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      Ptr<Packet> tail_p = m_packets.back();
      uint64_t tail_p_pr = GetPriorityFromPacket(tail_p);
      if(tail_p_pr > pr) {
        m_packets.pop_back();
        Drop (tail_p);
        m_totalpackets--;
        m_bytesInQueue -= tail_p->GetSize ();
        m_nPackets--;
        m_nBytes -= tail_p->GetSize ();
      } else {
        Drop (p);
        NS_LOG_LOGIC("Dropped the packet from queue 0");
        return false;
      }
  }

  if (m_mode == QUEUE_MODE_BYTES && (m_bytesInQueue + p->GetSize () >= m_maxBytes))
  {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      while((m_bytesInQueue + p->GetSize() >= m_maxBytes)&&(dropped)) {
        Ptr<Packet> tail_p = m_packets.back();
        tail_p_pr = GetPriorityFromPacket(tail_p);
        if(tail_p_pr > pr) {
          m_packets.pop_back();
          Drop (tail_p);
          m_totalpackets--;
          m_bytesInQueue -= tail_p->GetSize ();
          m_nPackets--;
          m_nBytes -= tail_p->GetSize ();
        } else {
          dropped = false;
        }
      }
      if((m_bytesInQueue + p->GetSize() >= m_maxBytes)&&(!dropped))
      {
        Drop (p);
        NS_LOG_LOGIC("Dropped the packet from queue 0");
        return false;
      }
  }

  if(m_backgrounddrop > 0)
  {
      Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
      double prob = uv->GetValue(0,1);
      
      
      if (prob < m_backgrounddrop)
      {
          NS_LOG_LOGIC ("background drop");
          //dropped = DropPacket(pr);
          //if(!dropped)
          //{
              Drop (p);
        
              NS_LOG_LOGIC("Dropped the packet from queue 0");
              return false;
         //}
      }
  } 

  

  //enqueue on basis of priority...also take priority 0 into consideration 
  InsertPacketInSortedQueue(p, pr);
  m_bytesInQueue += p->GetSize ();
  m_totalpackets++; 
  
  NS_LOG_INFO(Simulator::Now().GetSeconds()<<": "<<m_id<<"\t Enqueueing in queue "<<pr<<" at "<< GetTimestampFromPacket(p) <<" Total bytes in queue = "<<m_bytesInQueue);
  
  NS_LOG_LOGIC ("Number packets " << m_totalpackets);
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

Ptr<Packet>
LstfQueue::DoDequeue (void)
{

  //dequeue based on priority
  NS_LOG_FUNCTION (this<<m_id);

  while (!m_packets.empty ())
  {
          NS_LOG_LOGIC(Simulator::Now().GetSeconds()<<":Dequeuing from queue");
          Ptr<Packet> p = m_packets.front ();
          uint64_t pr = GetPriorityFromPacket(p);
          m_packets.pop_front ();
          m_bytesInQueue -= p->GetSize ();
          m_totalpackets--;

          NS_LOG_LOGIC ("Popped " << p);
          //p->Print(std::cout);
          //std::cout<<std::endl;     
          NS_LOG_LOGIC ("Number packets " << m_totalpackets);
          NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);
          NS_LOG_INFO(Simulator::Now().GetSeconds()<<": "<<m_id<<"\t Dequeueing with priority  "<< pr <<" Total bytes in subqueue = "<<m_bytesInQueue);
          return p;
  }
  NS_LOG_LOGIC ("All queues empty");
  return 0;

}

Ptr<const Packet>
LstfQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
  {
         NS_LOG_LOGIC ("Queue empty");
         return 0;
  }

  Ptr<Packet> p = m_packets.front ();

  NS_LOG_LOGIC ("Number packets " << m_totalpackets);
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);
  return p;
}





} // namespace ns3

