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
#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "rcp-queue.h"
#include "rcp-tag.h"
#include "ns3/random-variable-stream.h"
#define RTT 0.2
#define RTT_GAIN 0.02
#define FLOWSNUM 50

NS_LOG_COMPONENT_DEFINE ("RCPQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RCPQueue);

TypeId RCPQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::RCPQueue")
    .SetParent<Queue> ()
    .AddConstructor<RCPQueue> ()
    .AddAttribute ("Mode", 
                   "Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue size metric.",
                   EnumValue (QUEUE_MODE_BYTES),
                   MakeEnumAccessor (&RCPQueue::SetMode),
                   MakeEnumChecker (QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets accepted by this RCPQueue.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RCPQueue::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this RCPQueue.",
                   UintegerValue (100 * 65535),
                   MakeUintegerAccessor (&RCPQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Id", "The id (unique integer) of this Queue.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RCPQueue::m_id),
                   MakeUintegerChecker<uint32_t> ())   
    .AddAttribute ("Capacity", "Capacity of queue in bytes/sec",
                   DoubleValue (0),
                   MakeDoubleAccessor (&RCPQueue::m_capacity),
                   MakeDoubleChecker<double> ())  
  ;

  return tid;
}

void
RCPQueue::LogQueueLength()
{
  std::ofstream ofs("queuelength.txt", std::ios::app);
  ofs<<Simulator::Now().GetSeconds()<<"\t"<<m_id<<"\t"<<m_bytesInQueue<<"\n";
  ofs.close();
  m_sendEvent = Simulator::Schedule(Seconds(0.1), &RCPQueue::LogQueueLength, this);
}

RCPQueue::RCPQueue () :
  Queue (),
  m_packets (),
  m_bytesInQueue (0),
  m_capacity(0),
  m_virtualCapacity(0),
  m_alpha(0.4),
  m_beta(0.4),
  m_gamma(1),
  m_rate(0.05*125000000),
  m_inputTraffic(0),
  m_activeInputTraffic(0),
  m_trafficSpill(0),
  m_updateSlot(0.01),
  m_endSlot(0),
  m_Tq_rtt_sum(0),
  m_Tq_rtt(0),
  m_Tq_rtt_numPkts(0),
  m_input_traffic_rtt(0),
  m_rtt_moving_gain(RTT_GAIN),
  m_avg_rtt(RTT),
  m_numFlows(FLOWSNUM)
{
  NS_LOG_FUNCTION (this);
  m_Tq = min(RTT, m_updateSlot);
  LogQueueLength();

  Ptr<NormalRandomVariable> n = CreateObject<NormalRandomVariable> ();
  double T = n->GetValue(m_Tq, 0.2*m_Tq);
  if(T < 0)
    T = T*(-1);
  
  m_endSlot = m_Tq;
   
  m_queueTimeout = Simulator::Schedule(Seconds(m_Tq), &RCPQueue::QueueTimeout, this);
}

RCPQueue::~RCPQueue ()
{
  NS_LOG_FUNCTION (this);
}

void
RCPQueue::SetMode (RCPQueue::QueueMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

RCPQueue::QueueMode
RCPQueue::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}



void
RCPQueue::OnArrival (Ptr<Packet> p)
{
  uint32_t size = p->GetSize();
  double pkt_time = double(size)/m_capacity;
  double end_time = Simulator::Now().GetSeconds() + pkt_time;
  double part1, part2;

  RCPTag tag; 
  p->PeekPacketTag(tag);
  NS_ASSERT(tag.GetTypeId().GetName() == "ns3::RCPTag");  
  

  // update avg_rtt_ here
  double this_rtt = tag.GetRTT();

  if (this_rtt > 0)
  {
       m_Tq_rtt_sum += (this_rtt * size);
       m_input_traffic_rtt += size;
       m_Tq_rtt = running_avg(this_rtt, m_Tq_rtt, m_rate/m_capacity);
  }

  if (end_time <= m_endSlot)
    m_activeInputTraffic += size;
  else 
  {
    part2 = size * (end_time - m_endSlot)/pkt_time;
    part1 = size - part2;
    m_activeInputTraffic += part1;
    m_trafficSpill += part2;
  }
}

bool 
RCPQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  if (m_mode == QUEUE_MODE_PACKETS && (m_packets.size () >= m_maxPackets))
    {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      Drop (p);
      return false;
    }

  if (m_mode == QUEUE_MODE_BYTES && (m_bytesInQueue + p->GetSize () >= m_maxBytes))
    {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      Drop (p);
      return false;
    }

  
  m_bytesInQueue += p->GetSize ();
  m_packets.push (p);
  OnArrival(p);  

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

void
RCPQueue::OnDeparture (Ptr<Packet> p)
{
  RCPTag tag; 
  p->RemovePacketTag(tag);
  NS_ASSERT(tag.GetTypeId().GetName() == "ns3::RCPTag");  
  
  
  if((tag.GetBottleneckRate()==-1) || (tag.GetBottleneckRate() > m_rate))
      tag.SetBottleneckRate(m_rate);
  p->AddPacketTag(tag);

}



Ptr<Packet>
RCPQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();
  m_packets.pop ();
  m_bytesInQueue -= p->GetSize ();
  OnDeparture(p);

  NS_LOG_LOGIC ("Popped " << p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

Ptr<const Packet>
RCPQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

void
RCPQueue::QueueTimeout()
{
  double temp;
  //uint32_t Q_pkts;
  uint32_t Q_;

  double ratio;

  double virtual_link_capacity; // bytes per second

  m_inputTraffic = m_activeInputTraffic/m_Tq; // bytes per second

  Q_ = m_bytesInQueue;
  //Q_pkts = m_packets.size();

  if (m_input_traffic_rtt > 0)
    m_Tq_rtt_numPkts = m_Tq_rtt_sum/m_input_traffic_rtt;

   if (m_Tq_rtt_numPkts >= m_avg_rtt)
        m_rtt_moving_gain = (m_Tq/m_avg_rtt);
   else
        m_rtt_moving_gain = (m_rate/m_capacity)*(m_Tq_rtt_numPkts/m_avg_rtt)*(m_Tq/m_avg_rtt);

  m_avg_rtt = running_avg(m_Tq_rtt_numPkts, m_avg_rtt, m_rtt_moving_gain);
  
  virtual_link_capacity = m_gamma * m_capacity;

  ratio = (1 + ((m_Tq/m_avg_rtt)*(m_alpha*(virtual_link_capacity - m_inputTraffic) - m_beta*(Q_/m_avg_rtt)))/virtual_link_capacity);
  temp = m_rate * ratio;
  
  if (temp < 16000.0 )
  {    // Masayoshi 16 KB/sec = 128 Kbps
      m_rate = 16000.0;
      NS_LOG_INFO("Low");
  } 
  else if (temp > m_capacity)
  {
      m_rate = m_capacity;
      NS_LOG_INFO("High");
  } 
  else 
  {
    m_rate = temp;
  }

  NS_LOG_INFO(Simulator::Now().GetSeconds()<<"\t"<<m_id<<": Rate Changed to: "<<m_rate);

  m_Tq = min(m_avg_rtt, m_updateSlot);  
  m_Tq_rtt = 0;
  m_Tq_rtt_sum = 0;
  m_input_traffic_rtt = 0;
  m_activeInputTraffic = m_trafficSpill;
  m_trafficSpill = 0;
  m_endSlot = Simulator::Now().GetSeconds() + m_Tq;
 
 
  m_queueTimeout = Simulator::Schedule(Seconds(m_Tq), &RCPQueue::QueueTimeout, this);
}




double RCPQueue::running_avg(double var_sample, double var_last_avg, double gain)
{
  double avg;
  if (gain < 0)
    exit(3);
  avg = gain * var_sample + ( 1 - gain) * var_last_avg;
  return avg;
}


} // namespace ns3

