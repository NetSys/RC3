/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Adrian Sai-wah Tam
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
 * Author: Radhika Mittal, build upon code by Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#ifndef TCP_NEWRENO_H
#define TCP_NEWRENO_H

#include "tcp-socket-base.h"

namespace ns3 {

/**
 * \ingroup socket
 * \ingroup tcp
 *
 * \brief An implementation of a stream socket using TCP.
 *
 * This class contains the RCP implementation of TCP, as of RFC2582.
 */
class TcpRCP : public TcpSocketBase
{
public:
  static TypeId GetTypeId (void);
  /**
   * Create an unbound tcp socket.
   */
  TcpRCP (void);
  TcpRCP (const TcpRCP& sock);
  virtual ~TcpRCP (void);

  // From TcpSocketBase
  virtual int Connect (const Address &address);
  virtual int Listen (void);

protected:
  virtual uint32_t Window (void); // Return the max possible number of unacked bytes
  virtual Ptr<TcpSocketBase> Fork (void); // Call CopyObject<TcpRCP> to clone me
  virtual void NewAck (SequenceNumber32 const& seq); // Inc cwnd and call NewAck() of parent
  virtual void DupAck (const TcpHeader& t, uint32_t count);  // Halving cwnd and reset nextTxSequence
  virtual void Retransmit (void); // Exit fast recovery upon retransmit timeout
  virtual bool SendPendingData (bool withAck = false); // Send as much as the window allows (changed to virtual by Radhika)
  virtual uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck); // Send a data packet
  virtual void SendEmptyPacket (uint8_t flags); // Send a empty packet that carries a flag, e.g. ACK
  virtual void DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface); //Get a pkt from L3
  virtual void SendNextPacket();
  virtual void RateChange();

  // Implementing ns3::TcpSocket -- Attribute get/set
  virtual void     SetSegSize (uint32_t size);
  virtual void     SetSSThresh (uint32_t threshold);
  virtual uint32_t GetSSThresh (void) const;
  virtual void     SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;
private:
  void InitializeCwnd (void);            // set m_cWnd when connection starts

protected:
  TracedValue<uint32_t>  m_cWnd;         //< Congestion window
  uint32_t               m_ssThresh;     //< Slow Start Threshold
  uint32_t               m_initialCWnd;  //< Initial cWnd value
  SequenceNumber32       m_recover;      //< Previous highest Tx seqnum for fast recovery
  uint32_t               m_retxThresh;   //< Fast Retransmit threshold
  bool                   m_inFastRec;    //< currently in fast recovery
  bool                   m_limitedTx;    //< perform limited transmit

  uint32_t               m_flowid;

  //RCP variables
  double                 m_bottleneckRate;
  double                 m_reverseBottleneckRate;
  double                 m_rcprtt;
  double                 m_reverseTs;
  double                 m_seenTs;
  double                 m_interval;
  double                 m_lastpkttime;
  bool                   m_rcpRunning;
  EventId                m_sendEvent;
};

} // namespace ns3

#endif /* TCP_NEWRENO_H */
