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

#ifndef DROPTAIL_H
#define DROPTAIL_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"

#define NUM_PRIORITY_QUEUES 5    
//#define BUFSZ 1000000

namespace ns3 {

class TraceContainer;

//defining priority queue
class LstfQueue : public Queue {
public:
  static TypeId GetTypeId (void);
  /**
   * \brief LstfQueue Constructor
   *
   * Creates a droptail queue with a maximum size of 100 packets by default
   */
  LstfQueue ();

  virtual ~LstfQueue();

  /**
   * Set the operating mode of this device.
   *
   * \param mode The operating mode of this device.
   *
   */
  void SetMode (LstfQueue::QueueMode mode);

  /**
   * Get the encapsulation mode of this device.
   *
   * \returns The encapsulation mode of this device.
   */
  LstfQueue::QueueMode GetMode (void);

private:
  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;
  bool DropPacket(uint16_t);
  void InsertPacketInSortedQueue(Ptr<Packet>, int64_t pr);
  int64_t GetSlackFromPacket(Ptr<const Packet>);
  int64_t GetPriorityFromPacket(Ptr<const Packet>);
  uint64_t GetTimestampFromPacket(Ptr<const Packet>);
  void SetPriorityForPacket(Ptr<Packet>, int64_t pr);
  void SetTimestampForPacket(Ptr<Packet>);
  uint32_t GetIdFromPacket(Ptr<const Packet>);
  int64_t GetTransmissionTime(Ptr<const Packet>);
  void LogQueueLength();
  
  uint32_t counts[NUM_PRIORITY_QUEUES];
  std::deque<Ptr<Packet> > m_packets;
  uint32_t m_totalpackets;
  uint32_t m_maxPackets;
  uint32_t m_maxBytes;
  uint32_t m_bytesInQueue;
  uint32_t m_id;
  uint32_t m_bandwidth;
  double m_backgrounddrop;
  QueueMode m_mode;
  EventId m_sendEvent;
};




} // namespace ns3

#endif /* PRIORITY_H */
