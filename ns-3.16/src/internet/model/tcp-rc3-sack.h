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
 * Author: Radhika Mittal and Justine Sherry, build upon code by Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#ifndef TCP_AGGRESSIVESACK_H
#define TCP_AGGRESSIVESACK_H

#include "tcp-socket-base.h"
#include "scoreboard.h"
#define SACKSTACKSZ 3
#include <deque>
#include "ns3/queue.h"

namespace ns3 {

/**
 * \ingroup socket
 * \ingroup tcp
 *
 * \brief An implementation of a stream socket using TCP.
 *
 */
struct SackStackEntry
{
    SequenceNumber32 left;
      SequenceNumber32 right;
};


class TcpRC3Sack : public TcpSocketBase
{
public:
  static TypeId GetTypeId (void);
  /**
   * Create an unbound tcp socket.
   */
  TcpRC3Sack (void);
  TcpRC3Sack (const TcpRC3Sack& sock);
  virtual ~TcpRC3Sack (void);

  // From TcpSocketBase
  virtual int Connect (const Address &address);
  virtual int Listen (void);

protected:
  virtual uint32_t Window (void); // Return the max possible number of unacked bytes
  virtual Ptr<TcpSocketBase> Fork (void); // Call CopyObject<TcpRC3Sack> to clone me
  virtual void NewAck (SequenceNumber32 const& seq); // Inc cwnd and call NewAck() of parent
  virtual void DupAck (const TcpHeader& t, uint32_t count);  // Halving cwnd and reset nextTxSequence
  virtual void Retransmit (void); // Exit fast recovery upon retransmit timeout
   // Implementing ns3::TcpSocket -- Attribute get/set
  virtual void     SetSegSize (uint32_t size);
  virtual void     SetSSThresh (uint32_t threshold);
  virtual uint32_t GetSSThresh (void) const;
  virtual void     SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;

  int Close (void);   // Close by app: Kill socket after tx buffer emptied
  void ProcessListen (Ptr<Packet>, const TcpHeader&, const Address&, const Address&); // Process the newly received ACK
  virtual int Send (Ptr<Packet> p, uint32_t flags);  // Call by app to send data to network

  virtual void ProcessSynSent (Ptr<Packet> packet, const TcpHeader& tcpHeader);
  virtual void ProcessSynRcvd (Ptr<Packet>, const TcpHeader&, const Address&, const Address&); // Received a packet upon SYN_RCVD

  virtual void ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader);
  virtual void ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader);

  virtual uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck); // Send a data packet
  void SendEmptyPacket (uint8_t flags);
  virtual void SendEmptyPacket (uint8_t flags, uint64_t priority, bool withSack=false);
  virtual bool SendPendingData (bool withAck = false); // Send as much as the window allows

  virtual void FillSackStack (SequenceNumber32 seqno);
  virtual void PrintSackStack(std::ostream &os);


  void StartRLPLoop();
  bool SendLowPriorityPacket();


private:
  void InitializeCwnd (void);            // set m_cWnd when connection starts
  uint32_t               drops;
  bool                   rc3_p2;
  uint32_t               p2_block_max;
  uint32_t               p2_block_bytes_sent;
  uint32_t               last_block_seq;
  uint32_t               p2_window;
  uint32_t               p2_sent_count;
protected:
  TracedValue<uint32_t>  m_cWnd;         //< Congestion window
  uint32_t               m_ssThresh;     //< Slow Start Threshold
  uint32_t               m_initialCWnd;  //< Initial cWnd value
  SequenceNumber32       m_recover;      //< Previous highest Tx seqnum for fast recovery
  uint32_t               m_retxThresh;   //< Fast Retransmit threshold
  bool                   m_inFastRec;    //< currently in fast recovery
  bool                   m_limitedTx;    //< perform limited transmit
  bool                   m_limitedWindow; //limited by receive window?
  bool                   m_multipriorities; //use multiple priority levels
  bool                   m_rc3Once; //
  uint32_t               m_prioritySlots;
  bool                   m_logRTO;
  bool                   m_logAcks;
  bool                   m_logCleanUp;
  bool                   m_flushOut;
  bool                   m_lstf;
  Ptr<Queue>             m_devQueue;
  uint32_t               m_flowid;
  uint32_t               m_flowsize;
  uint8_t                m_priority;
  uint32_t               m_p2Window;

  SequenceNumber32       m_bumpedSeq;
  ScoreBoard             m_scoreboard;
  std::deque<SackStackEntry> m_sackstack;
};

} // namespace ns3

#endif /* TCP_AGGRESSIVE_H */
