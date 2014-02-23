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
#include "priority-queue.h"


#define MAXPACKETS 1000
#define MAXBYTES 225000


NS_LOG_COMPONENT_DEFINE ("PriorityQueue");

namespace ns3 {

//priority queue

NS_OBJECT_ENSURE_REGISTERED (PriorityQueue);

TypeId PriorityQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::PriorityQueue")
    .SetParent<Queue> ()
    .AddConstructor<PriorityQueue> ()
    .AddAttribute ("Mode", 
                   "Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue size metric.",
                   EnumValue (QUEUE_MODE_BYTES),
                   MakeEnumAccessor (&PriorityQueue::SetMode),
                   MakeEnumChecker (QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets accepted by this PriorityQueue.",
                   UintegerValue (MAXPACKETS),
                   MakeUintegerAccessor (&PriorityQueue::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this PriorityQueue.",
                   UintegerValue (MAXBYTES),
                   MakeUintegerAccessor (&PriorityQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Id", "The id (unique integer) of this Queue.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&PriorityQueue::m_id),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BackgroundDrop", "Background Drop rate",
                    DoubleValue(0.0),
                    MakeDoubleAccessor(&PriorityQueue::m_backgrounddrop),
                    MakeDoubleChecker<double> ())  
    ;
  return tid;
}

void
PriorityQueue::LogQueueLength()
{
  std::ofstream ofs("queuelength.txt", std::ios::app);
  ofs<<Simulator::Now().GetSeconds()<<"\t"<<m_id<<"\t"<<m_packets[0].size()<<"\t"<<m_totalpackets<<"\t"<<m_bytesInSubQueue[0]<<"\t"<<m_bytesInQueue<<"\n";
  ofs.close();
  m_sendEvent = Simulator::Schedule(Seconds(0.1), &PriorityQueue::LogQueueLength, this);
}

PriorityQueue::PriorityQueue () :
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
  for (int i=0; i< NUM_PRIORITY_QUEUES; i++){
    m_bytesInSubQueue[i] = 0;
    counts[i] = 0;
  }
  LogQueueLength();
}

PriorityQueue::~PriorityQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
PriorityQueue::SetMode (PriorityQueue::QueueMode mode)
{
  NS_LOG_FUNCTION (mode);
  m_mode = mode;
}

PriorityQueue::QueueMode
PriorityQueue::GetMode (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mode;
}


uint16_t 
PriorityQueue::GetPriorityFromPacket(Ptr <const Packet> p)
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
    return tag.GetPriority();
}

uint32_t 
PriorityQueue::GetIdFromPacket(Ptr <const Packet> p)
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



bool
PriorityQueue::DropPacket(uint16_t pr)
{
  uint32_t i;
  Ptr<Packet> p;
  for(i=NUM_PRIORITY_QUEUES-1;i>pr;i--)
  {
    if(m_packets[i].empty())
    {
      continue;
    }
 
    p = m_packets[i].back();
    m_packets[i].pop_back();
    
    
    
    Drop (p);
    m_totalpackets--;
    m_bytesInQueue -= p->GetSize ();
    m_bytesInSubQueue[i] -= p->GetSize ();
    m_nPackets--;
    m_nBytes -= p->GetSize ();
    
    NS_LOG_LOGIC("Dropped packet from queue"<<i+1);
    NS_LOG_ERROR("Dropped packet");
    
    return true;

  }
  return false;
}

void PriorityQueue::FlushOutFlowPackets(uint32_t flowId)
{

  //flushing out packets with priority flowId
  NS_LOG_FUNCTION (this<<m_id);

   for(int i=NUM_PRIORITY_QUEUES-1;i>=1;i--)
   {
      int j = 0;
      while((m_packets[i].begin() + j) != m_packets[i].end())
      {
        std::deque< Ptr<Packet> >::iterator it = m_packets[i].begin() + j;
        if(GetIdFromPacket(*it) == flowId)
        {
          NS_LOG_INFO("Erasing packet from queue "<<i);
          m_totalpackets--;
          m_bytesInQueue -= (*it)->GetSize ();
          m_bytesInSubQueue[i] -= (*it)->GetSize ();
          m_nPackets--;
          m_nBytes -= (*it)->GetSize ();
          m_packets[i].erase(m_packets[i].begin() + j);
        }
        else
            j++;
      }
   }

}


bool 
PriorityQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p << m_id);
  bool dropped = true;
  uint16_t pr;

  pr = GetPriorityFromPacket(p);

  NS_LOG_LOGIC("Enqueueing in priority queue");
  //p->Print(std::cout);
  //std::cout<<std::endl;

  if (m_mode == QUEUE_MODE_PACKETS && (m_totalpackets >= m_maxPackets))
  {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      dropped = DropPacket(pr);
      if(!dropped)
      {
        Drop (p);
        
        NS_LOG_LOGIC("Dropped the packet from queue 0");
        return false;
      }
  }

  if (m_mode == QUEUE_MODE_BYTES && (m_bytesInQueue + p->GetSize () >= m_maxBytes))
  {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      while((m_bytesInQueue + p->GetSize() >= m_maxBytes)&&(dropped))
        dropped = DropPacket(pr);
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
  if(pr>=NUM_PRIORITY_QUEUES)
      pr = NUM_PRIORITY_QUEUES-1;

  m_packets[pr].push_back(p);

  m_bytesInSubQueue[pr] += p->GetSize ();
  m_bytesInQueue += p->GetSize ();
  m_totalpackets++; 
  
  NS_LOG_INFO(Simulator::Now().GetSeconds()<<": "<<m_id<<"\t Enqueueing in queue "<<pr<<" Total bytes in subqueue = "<<m_bytesInSubQueue[pr]);
  
  NS_LOG_LOGIC ("Number packets " << m_totalpackets);
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

Ptr<Packet>
PriorityQueue::DoDequeue (void)
{

  //dequeue based on priority
  int i;
  NS_LOG_FUNCTION (this<<m_id);

  for(i=0;i<NUM_PRIORITY_QUEUES;i++)
  //for(i=NUM_PRIORITY_QUEUES-1; i>=0; i--)
  {
      while (!m_packets[i].empty ())
      {
          NS_LOG_LOGIC(Simulator::Now().GetSeconds()<<":Dequeuing from queue"<<i);
          Ptr<Packet> p = m_packets[i].front ();
          m_packets[i].pop_front ();
          m_bytesInQueue -= p->GetSize ();
          m_bytesInSubQueue[i] -= p->GetSize ();
          m_totalpackets--;

          NS_LOG_LOGIC ("Popped " << p);
          //p->Print(std::cout);
          //std::cout<<std::endl;     
          NS_LOG_LOGIC ("Number packets " << m_totalpackets);
          NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);
          counts[i]++;
          NS_LOG_INFO(Simulator::Now().GetSeconds()<<": "<<m_id<<"\t Dequeueing in queue "<<i<<" Total bytes in subqueue = "<<m_bytesInSubQueue[i]);
          return p;
      }
  }
  NS_LOG_LOGIC ("All queues empty");
  return 0;

}

Ptr<const Packet>
PriorityQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);
  int i;

  for(i=0;i<NUM_PRIORITY_QUEUES;i++)
  {
    if (m_packets[i].empty ())
    {
         NS_LOG_LOGIC ("Queue empty");
         continue;
    }

    Ptr<Packet> p = m_packets[i].front ();

    NS_LOG_LOGIC ("Number packets " << m_totalpackets);
    NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);
    return p;
  }
  NS_LOG_LOGIC ("All queues empty");
  return 0;


}





} // namespace ns3

