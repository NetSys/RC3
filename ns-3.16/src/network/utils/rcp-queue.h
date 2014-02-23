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

namespace ns3 {

class TraceContainer;

/**
 * \ingroup queue
 *
 * \brief A FIFO packet queue that drops tail-end packets on overflow
 */
class RCPQueue : public Queue {
public:
  static TypeId GetTypeId (void);
  /**
   * \brief RCPQueue Constructor
   *
   * Creates a droptail queue with a maximum size of 100 packets by default
   */
  RCPQueue ();

  virtual ~RCPQueue();

  /**
   * Set the operating mode of this device.
   *
   * \param mode The operating mode of this device.
   *
   */
  void SetMode (RCPQueue::QueueMode mode);

  /**
   * Get the encapsulation mode of this device.
   *
   * \returns The encapsulation mode of this device.
   */
  RCPQueue::QueueMode GetMode (void);

private:
  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;
  void LogQueueLength();
  void OnArrival(Ptr<Packet> p);
  void OnDeparture(Ptr<Packet> p);
  void QueueTimeout();

  //RCP helper functions
  double running_avg(double var_sample, double var_last_avg, double gain);
  inline double max(double d1, double d2){ if (d1 > d2){return d1;} else {return d2;} }
  inline double min(double d1, double d2){ if (d1 < d2){return d1;} else {return d2;} }

  std::queue<Ptr<Packet> > m_packets;
  uint32_t m_maxPackets;
  uint32_t m_maxBytes;
  uint32_t m_bytesInQueue;
  uint32_t m_id;
  QueueMode m_mode;
  EventId m_sendEvent;

  EventId m_queueTimeout;
  
  //parameters
  double m_capacity;
  double m_virtualCapacity;

  double m_alpha;
  double m_beta;
  double m_gamma;

  double m_rate;

  double m_inputTraffic;
  double m_activeInputTraffic;
  double m_trafficSpill;

  double m_updateSlot;
  double m_endSlot;
  double m_Tq;
  
  double m_Tq_rtt_sum;
  double m_Tq_rtt;
  double m_Tq_rtt_numPkts;
  uint32_t m_input_traffic_rtt;
  
  double m_rtt_moving_gain;
  double m_avg_rtt;
  double m_numFlows;
};

} // namespace ns3

#endif /* DROPTAIL_H */
