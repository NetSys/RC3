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
 * Author: Radhika Mittal and Justine Sherry; based on code by Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }
#include <cstdio>
#include <iostream>
#include <fstream>
#include "tcp-rc3-sack.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/packet.h"
#include "ns3/math.h"
#include "ns3/my-priority-tag.h"
#include "ns3/queue.h"
#include "ns3/priority-queue.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include "tcp-l4-protocol.h"
#include "tcp-option-sack.h"

using namespace std;

NS_LOG_COMPONENT_DEFINE ("TcpRC3Sack");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpRC3Sack);

TypeId
TcpRC3Sack::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpRC3Sack")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpRC3Sack> ()
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&TcpRC3Sack::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LimitedTransmit", "Enable limited transmit",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRC3Sack::m_limitedTx),
		    MakeBooleanChecker ())
    .AddAttribute ("LimitedWindow", "Limit window by 2*recv_window",
		    BooleanValue (true),
		    MakeBooleanAccessor (&TcpRC3Sack::m_limitedWindow),
		    MakeBooleanChecker ())
    .AddAttribute ("UseP2", "Use rc3 p2 traffic",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRC3Sack::rc3_p2),
		    MakeBooleanChecker ())
    .AddAttribute ("RC3Once", "Use rc3 p2 traffic only once",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRC3Sack::m_rc3Once),
		    MakeBooleanChecker ())
    .AddAttribute ("MultiPriorities", "Use multiple priority levels",
		    BooleanValue (true),
		    MakeBooleanAccessor (&TcpRC3Sack::m_multipriorities),
		    MakeBooleanChecker ())
    .AddAttribute ("PrioritySlots", "Priority Slots",
                    UintegerValue (4),
                    MakeUintegerAccessor (&TcpRC3Sack::m_prioritySlots),
                    MakeUintegerChecker<uint32_t> ()) 
    .AddAttribute ("FlowId", "Flow id",
                    UintegerValue (0),
                    MakeUintegerAccessor (&TcpRC3Sack::m_flowid),
                    MakeUintegerChecker<uint32_t> ()) 
    .AddAttribute ("FlowSize", "Flow Size",
                    UintegerValue (0),
                    MakeUintegerAccessor (&TcpRC3Sack::m_flowsize),
                    MakeUintegerChecker<uint32_t> ()) 
    .AddAttribute ("Priority", "Priority",
                    UintegerValue (0),
                    MakeUintegerAccessor (&TcpRC3Sack::m_priority),
                    MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("LogRTO", "Log RTO",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRC3Sack::m_logRTO),
		    MakeBooleanChecker ())
    .AddAttribute ("LogAcks", "Log Acks",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRC3Sack::m_logAcks),
		    MakeBooleanChecker ())
    .AddAttribute ("LogCleanUp", "Log CleanUp",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRC3Sack::m_logCleanUp),
		    MakeBooleanChecker ())
    .AddAttribute ("FlushOut", "Flush Out",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRC3Sack::m_flushOut),
		    MakeBooleanChecker ())
    .AddAttribute ("LSTF", "LSTF",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRC3Sack::m_lstf),
		    MakeBooleanChecker ())
    .AddAttribute("DeviceQueue", "Device Queue",
       PointerValue(), 
       MakePointerAccessor(&TcpRC3Sack::m_devQueue),     
        MakePointerChecker<Queue>())
  .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpRC3Sack::m_cWnd));
    return tid;
}

TcpRC3Sack::TcpRC3Sack (void)
  : drops(0),
    rc3_p2(false),
    p2_block_max(0),
    p2_block_bytes_sent(0),
    last_block_seq(0),
    m_retxThresh (3), // mute valgrind, actual value set by the attribute system
    m_inFastRec (false),
    m_limitedTx (false), // mute valgrind, actual value set by the attribute system
    m_limitedWindow (true), // mute valgrind, actual value set by the attribute system
    m_multipriorities (true), // mute valgrind, actual value set by the attribute system
    m_rc3Once (false), // mute valgrind, actual value set by the attribute system
    m_prioritySlots(4),
    m_logRTO (false), // mute valgrind, actual value set by the attribute system
    m_logAcks (false), // mute valgrind, actual value set by the attribute system
    m_logCleanUp (false), // mute valgrind, actual value set by the attribute system
    m_flushOut (false), // mute valgrind, actual value set by the attribute system
    m_lstf (false), // mute valgrind, actual value set by the attribute system
    m_devQueue(0),
    m_flowid(0),
    m_flowsize(0),
    m_priority(0),
    m_bumpedSeq(0)
    {
      NS_LOG_FUNCTION (this);
    }

TcpRC3Sack::TcpRC3Sack (const TcpRC3Sack& sock)
  : TcpSocketBase (sock),
    drops(0),
    rc3_p2(false), 
    p2_block_max(0),
    p2_block_bytes_sent(0),
    last_block_seq(0),
    m_cWnd (sock.m_cWnd),
    m_ssThresh (sock.m_ssThresh),
    m_initialCWnd (sock.m_initialCWnd),
    m_retxThresh (sock.m_retxThresh),
    m_inFastRec (false),
    m_limitedTx (sock.m_limitedTx),
    m_limitedWindow (true), // mute valgrind, actual value set by the attribute system
    m_multipriorities (true), // mute valgrind, actual value set by the attribute system
    m_rc3Once (false), // mute valgrind, actual value set by the attribute system
    m_prioritySlots(4),
    m_logRTO (false), // mute valgrind, actual value set by the attribute system
    m_logAcks (false), // mute valgrind, actual value set by the attribute system
    m_logCleanUp (false), // mute valgrind, actual value set by the attribute system
    m_flushOut (false), // mute valgrind, actual value set by the attribute system
    m_lstf (false), // mute valgrind, actual value set by the attribute system
    m_devQueue(0),
    m_flowid(0),
    m_flowsize(0),
    m_priority(0),
    m_bumpedSeq(0)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpRC3Sack::~TcpRC3Sack (void)
{
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpRC3Sack::Listen (void)
{
  NS_LOG_FUNCTION (this);
  InitializeCwnd ();
  return TcpSocketBase::Listen ();
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpRC3Sack::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  InitializeCwnd ();
  return TcpSocketBase::Connect (address);
}


//Changed from Tcp-NewReno

/** Limit the size of in-flight data by cwnd and receiver's rxwin */
uint32_t
TcpRC3Sack::Window (void)
{
  NS_LOG_FUNCTION (this);
  if(m_limitedWindow)
      return std::min ((uint32_t) 2*m_rWnd.Get(), m_cWnd.Get ()); 
  return m_cWnd.Get ();
}

Ptr<TcpSocketBase>
TcpRC3Sack::Fork (void)
{
  return CopyObject<TcpRC3Sack> (this);
}

/** New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
void
TcpRC3Sack::NewAck (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("TcpRC3Sack receieved ACK for seq " << seq <<
                " cwnd " << m_cWnd <<
                " ssthresh " << m_ssThresh);
  // Check for exit condition of fast recovery
  if (m_inFastRec && seq < m_recover)
    { // Partial ACK, partial window deflation (RFC2582 sec.3 bullet #5 paragraph 3)
      m_cWnd -= seq - m_txBuffer.HeadSequence ();
      m_cWnd += m_segmentSize;  // increase cwnd
      NS_LOG_INFO ("Partial ACK in fast recovery: cwnd set to " << m_cWnd);

      /***SACK CHANGE***/
      m_scoreboard.m_highReTx = seq;
      /*****************/

      TcpSocketBase::NewAck (seq); // update m_nextTxSequence and send new data if allowed by window
     drops++;
      DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
      return;
    }
  else if (m_inFastRec && seq >= m_recover)
    { // Full ACK (RFC2582 sec.3 bullet #5 paragraph 2, option 1)
      m_cWnd = std::min (m_ssThresh, BytesInFlight () + m_segmentSize);
      m_inFastRec = false;
      NS_LOG_INFO ("Received full ACK. Leaving fast recovery with cwnd set to " << m_cWnd);
    }

  // Increase of cwnd based on current phase (slow start or congestion avoidance)
  if (m_cWnd < m_ssThresh)
    { // Slow start mode, add one segSize to cWnd. Default m_ssThresh is 65535. (RFC2001, sec.1)
      m_cWnd += m_segmentSize;
      if(TraceMe) std::cout << Simulator::Now().GetSeconds() << " " << GetNode()->GetId() << "(" << (int32_t) m_priority << ") AGGRESSIVE CWND SLOWSTART " << m_cWnd << " " << Window() <<  std::endl;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }
  else
    { // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
      // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
        double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ();
        adder = std::max (1.0, adder);
        m_cWnd += static_cast<uint32_t> (adder);
      
      if(TraceMe) std::cout << Simulator::Now().GetSeconds() << " " <<  GetNode()->GetId() << "(" << (int32_t) m_priority << ") CWND INCR " << m_cWnd << " " << Window() << std::endl;
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }

  // Complete newAck processing
  TcpSocketBase::NewAck (seq);
}


/** Cut cwnd and enter fast recovery mode upon triple dupack */
void
TcpRC3Sack::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  if (count == m_retxThresh && !m_inFastRec)
    { // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
      m_ssThresh = std::max (2 * m_segmentSize, static_cast<uint32_t> (BytesInFlight () / 2.0)); 
      m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_recover = m_highTxMark;
      m_inFastRec = true;

    if(TraceMe) std::cout << Simulator::Now().GetSeconds() << " " << GetNode()->GetId() << "(" << (int32_t) m_priority << ") CWND DROP " << m_cWnd << " " << Window() << std::endl;

      NS_LOG_INFO ("Triple dupack. Enter fast recovery mode. Reset cwnd to " << m_cWnd <<
                   ", ssthresh to " << m_ssThresh << " at fast recovery seqnum " << m_recover << " bytes in flight: " << BytesInFlight () / 2.0);
      drops++;
      DoRetransmit ();

      /***SACK CHANGE***/
      m_scoreboard.m_highReTx = t.GetAckNumber();
      /*****************/

    }
  else if (m_inFastRec)
    { // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
      m_cWnd += m_segmentSize;
      NS_LOG_INFO ("Dupack in fast recovery mode. Increase cwnd to " << m_cWnd);
      SendPendingData (m_connected);
    }
  else if (!m_inFastRec && m_limitedTx && m_txBuffer.SizeFromSequence (m_nextTxSequence) > 0)
    { // RFC3042 Limited transmit: Send a new packet for each duplicated ACK before fast retransmit
      NS_LOG_INFO ("Limited transmit");
      uint32_t sz = SendDataPacket (m_nextTxSequence, m_segmentSize, true);
      m_nextTxSequence += sz;                    // Advance next tx sequence
    };
}

/** Retransmit timeout */
void
TcpRC3Sack::Retransmit (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
  m_inFastRec = false;

  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT) return;
  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer.HeadSequence () >= m_highTxMark) return;

  // According to RFC2581 sec.3.1, upon RTO, ssthresh is set to half of flight
  // size and cwnd is set to 1*MSS, then the lost packet is retransmitted and
  // TCP back to slow start
 
  /***SACK Changes***/
  m_scoreboard.ClearScoreBoard();
  /*****************/ 

  m_ssThresh = std::max (2 * m_segmentSize, static_cast<uint32_t> (BytesInFlight () / 2.0));
  m_cWnd = m_segmentSize;
  //std::cout << this << " Reducing TxSequence to " << m_txBuffer.HeadSequence() << " from " << m_nextTxSequence << std::endl;
  m_nextTxSequence = m_txBuffer.HeadSequence (); // Restart from highest Ack

  NS_LOG_INFO ("RTO. Reset cwnd to " << m_cWnd <<
               ", ssthresh to " << m_ssThresh << ", restart from seqnum " << m_nextTxSequence);
  if(m_logRTO)
  {
    ofstream ofs("rto.txt", ios::app);
    ofs<<Simulator::Now().GetSeconds()<<"\t"<<m_flowid<<"\t"<<(uint16_t)m_priority<<"\n";
    ofs.close();
  }
  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  drops++;
  DoRetransmit ();                          // Retransmit the packet
}



void
TcpRC3Sack::SetSegSize (uint32_t size)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpRC3Sack::SetSegSize() cannot change segment size after connection started.");
  m_segmentSize = size;
}

void
TcpRC3Sack::SetSSThresh (uint32_t threshold)
{
  m_ssThresh = threshold;
}

uint32_t
TcpRC3Sack::GetSSThresh (void) const
{
  return m_ssThresh;
}

void
TcpRC3Sack::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpRC3Sack::SetInitialCwnd() cannot change initial cwnd after connection started.");
  m_initialCWnd = cwnd;
}

uint32_t
TcpRC3Sack::GetInitialCwnd (void) const
{
  return m_initialCWnd;
}

void 
TcpRC3Sack::InitializeCwnd (void)
{
  /*
   * Initialize congestion window, default to 1 MSS (RFC2001, sec.1) and must
   * not be larger than 2 MSS (RFC2581, sec.3.1). Both m_initiaCWnd and
   * m_segmentSize are set by the attribute system in ns3::TcpSocket.
   */
  m_cWnd = m_initialCWnd * m_segmentSize;
}

//Changed from Tcp-Socket-Base
int
TcpRC3Sack::Close (void)
{
  NS_LOG_FUNCTION (this);
  // First we check to see if there is any unread rx data
  // Bug number 426 claims we should send reset in this case.
  if (m_rxBuffer.Size () != 0)
    { 
      SendRST ();
  //    return 0;
    }    
  if (m_txBuffer.SizeFromSequence (m_nextTxSequence) > 0) 
    { // App close with pending data must wait until all data transmitted
      m_txBuffer.DiscardUpTo(m_txBuffer.TailSequence());
  }  
  return DoClose ();
}


//Changed from socket-base
int
TcpRC3Sack::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p);
  NS_ABORT_MSG_IF (flags, "use of flags is not supported in TcpSocketBase::Send()");
  if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
    {    
      // Store the packet into Tx buffer
      if (!m_txBuffer.Add (p)) 
        { // TxBuffer overflow, send failed
          m_errno = ERROR_MSGSIZE;
          return -1;
        }    
      // Submit the data to lower layers
      NS_LOG_LOGIC ("txBufSize=" << m_txBuffer.Size () << " state " << TcpStateName[m_state]);
      if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
        { // Try to send the data out
          SendPendingData (m_connected);
          if(!m_rc3Once)
              StartRLPLoop();
        }    
      return p->GetSize ();
    }    
  else 
    { // Connection not established yet
      m_errno = ERROR_NOTCONN;
      return -1; // Send failure
    }    
}


//Changed from Tcp-Socket-Base
void
TcpRC3Sack::ProcessListen (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                              const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  // Fork a socket if received a SYN. Do nothing otherwise.
  // C.f.: the LISTEN part in tcp_v4_do_rcv() in tcp_ipv4.c in Linux kernel
  if (tcpflags != TcpHeader::SYN)
    {    
      return;
    }    

  // Call socket's notify function to let the server app know we got a SYN
  // If the server app refuses the connection, do nothing
  if (!NotifyConnectionRequest (fromAddress))
    {    
      return;
    }    

  MyPriorityTag tag;
  bool peeked = packet->PeekPacketTag(tag);
  NS_ASSERT(peeked);
  NS_ASSERT(tag.GetTypeId().GetName() == "ns3::MyPriorityTag");

  uint32_t id = 0;
  uint64_t pr = 0;

  if(peeked)
  {
    id = tag.GetId();
    pr = tag.GetPriority();
  }

  // Clone the socket, simulate fork
  Ptr<TcpSocketBase> newSock = Fork ();
  NS_LOG_LOGIC ("Cloned a TcpSocketBase " << newSock);
  newSock->SetAttribute("FlowId", UintegerValue (id));
  newSock->SetAttribute("Priority",UintegerValue (pr));
  Simulator::ScheduleNow (&TcpSocketBase::CompleteFork, newSock,
                          packet, tcpHeader, fromAddress, toAddress);
}


/** Received a packet upon SYN_SENT */
void
TcpRC3Sack::ProcessSynSent (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  /****SACK change****/
  //initializing score board
  m_scoreboard.CreateScoreBoard(m_flowsize, m_segmentSize, m_retxThresh);
  //m_scoreboard.Print(std::cerr, m_nextTxSequence, m_txBuffer.HeadSequence());
  /******************/

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == 0)
    { // Bare data, accept it and move to ESTABLISHED state. This is not a normal behaviour. Remove this?
      NS_LOG_INFO ("SYN_SENT -> ESTABLISHED");
      m_state = ESTABLISHED;
      m_connected = true;
      m_retxEvent.Cancel ();
      m_delAckCount = m_delAckMaxCount;
      ReceivedData (packet, tcpHeader);
      Simulator::ScheduleNow (&TcpSocketBase::ConnectionSucceeded, this);
      StartRLPLoop();
    }
  else if (tcpflags == TcpHeader::ACK)
    { // Ignore ACK in SYN_SENT
    }
  else if (tcpflags == TcpHeader::SYN)
    { // Received SYN, move to SYN_RCVD state and respond with SYN+ACK
      NS_LOG_INFO ("SYN_SENT -> SYN_RCVD");
      m_state = SYN_RCVD;
      m_cnCount = m_cnRetries;
      m_rxBuffer.SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
    }
  else if (tcpflags == (TcpHeader::SYN | TcpHeader::ACK)
           && m_nextTxSequence + SequenceNumber32 (1) == tcpHeader.GetAckNumber ())
    { // Handshake completed
      NS_LOG_INFO ("SYN_SENT -> ESTABLISHED");
      m_state = ESTABLISHED;
      m_connected = true;
      m_retxEvent.Cancel ();
      m_rxBuffer.SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      m_highTxMark = ++m_nextTxSequence;
      m_txBuffer.SetHeadSequence (m_nextTxSequence);
      SendEmptyPacket (TcpHeader::ACK);
      SendPendingData (m_connected);
      Simulator::ScheduleNow (&TcpSocketBase::ConnectionSucceeded, this);
      StartRLPLoop ();
      // Always respond to first data packet to speed up the connection.
      // Remove to get the behaviour of old NS-3 code.
      m_delAckCount = m_delAckMaxCount;
    }
  else
    { // Other in-sequence input
      if (tcpflags != TcpHeader::RST)
        { // When (1) rx of FIN+ACK; (2) rx of FIN; (3) rx of bad flags
          NS_LOG_LOGIC ("Illegal flag " << std::hex << static_cast<uint32_t> (tcpflags) << std::dec << " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
    }
}

/** Received a packet upon SYN_RCVD */
void
TcpRC3Sack::ProcessSynRcvd (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                               const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  /****SACK change****/
  //initializing score board
  m_scoreboard.CreateScoreBoard(m_txBuffer.Size(), m_segmentSize, m_retxThresh);
  /*******************/ 

  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == 0
      || (tcpflags == TcpHeader::ACK
          && m_nextTxSequence + SequenceNumber32 (1) == tcpHeader.GetAckNumber ()))
    { // If it is bare data, accept it and move to ESTABLISHED state. This is
      // possibly due to ACK lost in 3WHS. If in-sequence ACK is received, the
      // handshake is completed nicely.
      NS_LOG_INFO ("SYN_RCVD -> ESTABLISHED");
      m_state = ESTABLISHED;
      m_connected = true;
      m_retxEvent.Cancel ();
      m_highTxMark = ++m_nextTxSequence;
      m_txBuffer.SetHeadSequence (m_nextTxSequence);
      
      if (m_endPoint)
        {
          m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                               InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
        }
      else if (m_endPoint6)
        {
          m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
        }
      // Always respond to first data packet to speed up the connection.
      // Remove to get the behaviour of old NS-3 code.
      m_delAckCount = m_delAckMaxCount;
      ReceivedAck (packet, tcpHeader);
      NotifyNewConnectionCreated (this, fromAddress);
      // As this connection is established, the socket is available to send data now
      if (GetTxAvailable () > 0)
        {
      
          NotifySend (GetTxAvailable ());
          StartRLPLoop();
        }
    }
  else if (tcpflags == TcpHeader::SYN)
    { // Probably the peer lost my SYN+ACK
      m_rxBuffer.SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
    }
  else if (tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
    {
      if (tcpHeader.GetSequenceNumber () == m_rxBuffer.NextRxSequence ())
        { // In-sequence FIN before connection complete. Set up connection and close.
          m_connected = true;
          m_retxEvent.Cancel ();
          m_highTxMark = ++m_nextTxSequence;
          m_txBuffer.SetHeadSequence (m_nextTxSequence);
          if (m_endPoint)
            {
              m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                   InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          else if (m_endPoint6)
            {
              m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                    Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          PeerClose (packet, tcpHeader);
        }
    }
  else
    { // Other in-sequence input
      if (tcpflags != TcpHeader::RST)
        { // When (1) rx of SYN+ACK; (2) rx of FIN; (3) rx of bad flags
          NS_LOG_LOGIC ("Illegal flag " << tcpflags << " received. Reset packet is sent.");
          if (m_endPoint)
            {
              m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                   InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          else if (m_endPoint6)
            {
              m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                    Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          SendRST ();
        }
      CloseAndNotify ();
    }
}

/****SACK change****/
void
TcpRC3Sack::FillSackStack(SequenceNumber32 seqno)
{
    //find left and right edges
    SequenceNumber32 leftEdge = seqno;
    while(leftEdge > SequenceNumber32(1))
    {
        if(m_rxBuffer.m_data.find(leftEdge - m_segmentSize) != m_rxBuffer.m_data.end())
            leftEdge = leftEdge - m_segmentSize;
        else
            break;
    } 
   


    SequenceNumber32 rightEdge = seqno + m_segmentSize;
    while(1)
    {
        if(m_rxBuffer.m_data.find(rightEdge) != m_rxBuffer.m_data.end())
            rightEdge = rightEdge + m_segmentSize;
        else
            break;
    } 

    //TODO: Take care of case where packets are not multiples of MSS
    
    
    SackStackEntry ss_entry;
    ss_entry.left = leftEdge;
    ss_entry.right = rightEdge;

    //delete redundant entries
    for(int i=0; i<(int)m_sackstack.size(); i++)
    {
        if((m_sackstack[i].left >= leftEdge) && (m_sackstack[i].right <= rightEdge))
        {
            m_sackstack.erase(m_sackstack.begin()+i);
            i--;
        }

    }

    //push latest
    if(leftEdge > SequenceNumber32(1))
        m_sackstack.push_front(ss_entry);

    //pop extra
    /*while(m_sackstack.size() > 3)
        m_sackstack.pop_back();*/

}

void
TcpRC3Sack::PrintSackStack(std::ostream &os)
{
  os<<"______________";
  os<<"Size of Sack Stack"<<m_sackstack.size()<<"\n";
  for(int i=0; i < (int)m_sackstack.size(); i++)
    os<<"Left: "<<m_sackstack[i].left<<"\t Right: "<<m_sackstack[i].right<<"\n";
  os<<"______________";
}
/*************************/


// Receipt of new packet, put into Rx buffer
void
TcpRC3Sack::ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader)
{

  MyPriorityTag tag;
  bool peeked = p->PeekPacketTag(tag);
  NS_ASSERT(peeked);
  NS_ASSERT(tag.GetTypeId().GetName() == "ns3::MyPriorityTag");
  
  SequenceNumber32 expectedSeq = m_rxBuffer.NextRxSequence ();
  NS_LOG_INFO("Received Data "<<tcpHeader.GetSequenceNumber()<<"\t"<<(uint64_t)tag.GetPriority()<<"\n");
  
  if(tag.GetPriority() > 0){ //

    //std::cout << "RECEIVED LOW PRIORITY PACKET. " << tcpHeader.GetSequenceNumber() << std::endl;
    if(!m_rxBuffer.Add (p, tcpHeader))
    { // Insert failed: No data or RX buffer full
      //std::cout << "No room for p2 packet!" << std::endl;
    }

    /****SACK change****/
    FillSackStack(tcpHeader.GetSequenceNumber());  
    //PrintSackStack(std::cerr);
    /*******************/ 

    SendEmptyPacket (TcpHeader::ACK, tag.GetPriority(), true);

  }else{
      //std::cout << "RECEIVED HI PRIORITY PACKET. " << tcpHeader.GetSequenceNumber() << std::endl;


    NS_LOG_FUNCTION (this << tcpHeader);
    NS_LOG_LOGIC ("seq " << tcpHeader.GetSequenceNumber () <<
                  " ack " << tcpHeader.GetAckNumber () <<
                  " pkt size " << p->GetSize () );

    // Put into Rx buffer
    if (!m_rxBuffer.Add (p, tcpHeader))
      { // Insert failed: No data or RX buffer full
        SendEmptyPacket (TcpHeader::ACK);
        return;
      }
    // Now send a new ACK packet acknowledging all received and delivered data
    if (m_rxBuffer.Size () > m_rxBuffer.Available () || m_rxBuffer.NextRxSequence () > expectedSeq + p->GetSize ())
      { // A gap exists in the buffer, or we filled a gap: Always ACK
    
        /****SACK change****/
        FillSackStack(tcpHeader.GetSequenceNumber());  
        //PrintSackStack(std::cerr);
        /*******************/ 
        
        SendEmptyPacket (TcpHeader::ACK, 0, true);
      }
    else
      { // In-sequence packet: ACK if delayed ack count allows
        if (++m_delAckCount >= m_delAckMaxCount)
          {
            m_delAckEvent.Cancel ();
            m_delAckCount = 0;
            SendEmptyPacket (TcpHeader::ACK);
          }
        else if (m_delAckEvent.IsExpired ())
          {
            m_delAckEvent = Simulator::Schedule (m_delAckTimeout,
                                                 &TcpSocketBase::DelAckTimeout, this);
            NS_LOG_LOGIC (this << " scheduled delayed ACK at " << (Simulator::Now () + Simulator::GetDelayLeft (m_delAckEvent)).GetSeconds ());
          }
      }
  }

  // Notify app to receive if necessary
  if (expectedSeq < m_rxBuffer.NextRxSequence ())
    { // NextRxSeq advanced, we have something to send to the app
      if (!m_shutdownRecv)
        {
          NotifyDataRecv ();
        }
      // Handle exceptions
      if (m_closeNotified)
        {
          NS_LOG_WARN ("Why TCP " << this << " got data after close notification?");
        }
      // If we received FIN before and now completed all "holes" in rx buffer,
      // invoke peer close procedure
      if (m_rxBuffer.Finished () && (tcpHeader.GetFlags () & TcpHeader::FIN) == 0)
        {
          DoPeerClose ();
        }
    }

}

/** Process the newly received ACK */
void
TcpRC3Sack::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  /****SACK change****/
  m_scoreboard.UpdateScoreBoard(tcpHeader);
  /******************/

  Ptr<Packet> p = packet;

  MyPriorityTag tag;
  bool peeked = p->PeekPacketTag(tag);
  NS_ASSERT(peeked);
  NS_ASSERT(tag.GetTypeId().GetName() == "ns3::MyPriorityTag");
    
  if(tag.GetPriority() > 0){ 
    /****SACK change****/
    //m_scoreboard.Print(std::cerr, m_nextTxSequence, m_txBuffer.HeadSequence());
    /******************/
    return;
  }else if (tag.GetPriority() == 0){
    //std::cout <<  Simulator::Now().GetSeconds() << " " << this << " RECEIVED HI ACK. " << tcpHeader.GetAckNumber() << " " << m_txBuffer.Size() << std::endl;
  }else{
    std::cout << "Priorityless ACK?? Not possible" << std::endl;
  }
 

  
  NS_LOG_INFO("Received Ack "<<tcpHeader.GetAckNumber()<<"\t"<<(uint64_t)tag.GetPriority()<<"\t"<<m_nextTxSequence<<"\t"<<m_txBuffer.HeadSequence()<<"\t"<<m_highTxMark<<"\n");

  if(m_logAcks)
  {
    ofstream ofs("acks.txt", ios::app);
    if(m_flowid == 0)
        ofs<<Simulator::Now().GetSeconds()<<"\t"<<tcpHeader.GetAckNumber()<<"\n";
    ofs.close();
  }

  if((rc3_p2) && (tcpHeader.GetAckNumber() >= SequenceNumber32(p2_block_max)) && (tcpHeader.GetAckNumber() != m_bumpedSeq))
  {
    NS_LOG_INFO("All segments so far have been acked (ack = "<<tcpHeader.GetAckNumber()<<", max seq = "<<SequenceNumber32(p2_block_max)<<", bumped seq = "<<m_bumpedSeq<<"...time to clear low priority buffers");
    if(m_flushOut)
    {
      Queue* q = PeekPointer<Queue>(m_devQueue);
      PriorityQueue* pq = (PriorityQueue*) q;
      pq->FlushOutFlowPackets(m_flowid);
    }
  }
  //Received ACK. Compare the ACK number against highest unacked seqno
  if (0 == (tcpHeader.GetFlags () & TcpHeader::ACK))
    { // Ignore if no ACK flag
    }
  else if (tcpHeader.GetAckNumber () < m_txBuffer.HeadSequence ())
    { // Case 1: Old ACK, ignored.
      NS_LOG_LOGIC ("Ignored ack of " << tcpHeader.GetAckNumber ());
    }
  else if ((tcpHeader.GetAckNumber () == m_txBuffer.HeadSequence ()))
    { // Case 2: Potentially a duplicated ACK
      if(((tcpHeader.GetAckNumber() != m_bumpedSeq) && (m_bumpedSeq > SequenceNumber32(0))) || (m_bumpedSeq == SequenceNumber32(0)) || (!rc3_p2))
      {
        if (tcpHeader.GetAckNumber () < m_nextTxSequence && packet->GetSize() == 0)
          {
            NS_LOG_LOGIC ("Dupack of " << tcpHeader.GetAckNumber ());
            DupAck (tcpHeader, ++m_dupAckCount);
          }
        // otherwise, the ACK is precisely equal to the nextTxSequence, we need a jump
        if((tcpHeader.GetAckNumber () > m_nextTxSequence) && (m_nextTxSequence != SequenceNumber32(0))){
            NS_LOG_INFO("ACK BUMP case1! Win! " << tcpHeader.GetAckNumber() << " " << m_nextTxSequence); //JUSTINE
            if(m_logCleanUp)
            {
               ofstream ofs("cleanup.txt", ios::app);
              ofs<<Simulator::Now().GetSeconds()<<"\t"<<m_flowid<<"\t"<<(m_txBuffer.TailSequence() - 1)<<"\n";
              ofs.close();
            }

            m_bumpedSeq = tcpHeader.GetAckNumber(); 
            m_nextTxSequence = tcpHeader.GetAckNumber();
            m_highTxMark = std::max(m_highTxMark, m_nextTxSequence);
            NewAck(tcpHeader.GetAckNumber());
            return;
        }
      }
      else if ((m_bumpedSeq > SequenceNumber32(0)) && (tcpHeader.GetAckNumber() == m_bumpedSeq) && (rc3_p2))
      {
        NS_LOG_INFO("In bumped up phase (assuming all same acks in this phase are new)! " << tcpHeader.GetAckNumber() << " " << m_txBuffer.HeadSequence()<< " " << m_nextTxSequence ); 
       
       m_highTxMark = std::max(m_highTxMark, m_nextTxSequence);
       NewAck(tcpHeader.GetAckNumber());
       return;
      }
    }
  else if ((tcpHeader.GetAckNumber () > m_txBuffer.HeadSequence ()) && (m_nextTxSequence != SequenceNumber32(0)))
    { // Case 3: New ACK, reset m_dupAckCount and update m_txBuffer
      NS_LOG_LOGIC ("New ack of " << tcpHeader.GetAckNumber ());

      //if Ack is greater than next sequence as well as head sequence, we need a jump
      if(tcpHeader.GetAckNumber () > m_nextTxSequence){
          NS_LOG_INFO("ACK BUMP case2 (normal)! Win! " << tcpHeader.GetAckNumber() << " " << m_nextTxSequence); //JUSTINE
          if(m_logCleanUp)
          {
             ofstream ofs("cleanup.txt", ios::app);
             ofs<<Simulator::Now().GetSeconds()<<"\t"<<m_flowid<<"\t"<<(m_txBuffer.TailSequence() - 1)<<"\n";
             ofs.close();
          }

          m_bumpedSeq = tcpHeader.GetAckNumber();
          m_nextTxSequence = tcpHeader.GetAckNumber();
          m_highTxMark = std::max(m_highTxMark, m_nextTxSequence);
          //NewAck(tcpHeader.GetAckNumber());
          //return;
      }
      NewAck (tcpHeader.GetAckNumber ());
      m_dupAckCount = 0;

    }
  // If there is any data piggybacked, store it into m_rxBuffer
  if (packet->GetSize () > 0)
    {
      ReceivedData (packet, tcpHeader);
    }

    /****SACK change****/
    //m_scoreboard.Print(std::cerr, m_nextTxSequence, m_txBuffer.HeadSequence());
    /******************/
}

/** Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
    TCP header, and send to TcpL4Protocol */
uint32_t
TcpRC3Sack::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{
  NS_LOG_FUNCTION (this << seq << maxSize << withAck);
  Ptr<Packet> p = m_txBuffer.CopyFromSequence (maxSize, seq);
  uint32_t sz = p->GetSize (); // Size of packet
  uint8_t flags = withAck ? TcpHeader::ACK : 0; 
  uint32_t remainingData = m_txBuffer.SizeFromSequence (seq + SequenceNumber32 (sz));

  MyPriorityTag tag;
  tag.SetId(m_flowid);
  tag.SetPriority(0);
  p -> AddPacketTag(tag);

  NS_LOG_INFO(this<<" I am here, sending data "<<m_flowid<<"\t"<<(uint64_t)m_priority<<"\t"<<seq);
  /*
   * Add tags for each socket option.
   * Note that currently the socket adds both IPv4 tag and IPv6 tag
   * if both options are set. Once the packet got to layer three, only
   * the corresponding tags will be read.
   */
  if (IsManualIpTos ())
    {    
      SocketIpTosTag ipTosTag;
      ipTosTag.SetTos (GetIpTos ()); 
      p->AddPacketTag (ipTosTag);
    }    

  if (IsManualIpv6Tclass ())
    {    
      SocketIpv6TclassTag ipTclassTag;
      ipTclassTag.SetTclass (GetIpv6Tclass ()); 
      p->AddPacketTag (ipTclassTag);
    }

  if (IsManualIpTtl ())
    {
      SocketIpTtlTag ipTtlTag;
      ipTtlTag.SetTtl (GetIpTtl ());
      p->AddPacketTag (ipTtlTag);
    }

  if (IsManualIpv6HopLimit ())
    {
      SocketIpv6HopLimitTag ipHopLimitTag;
      ipHopLimitTag.SetHopLimit (GetIpv6HopLimit ());
      p->AddPacketTag (ipHopLimitTag);
    }

  if (m_closeOnEmpty && (remainingData == 0))
    {
      flags |= TcpHeader::FIN;
      if (m_state == ESTABLISHED)
        { // On active close: I am the first one to send FIN
          NS_LOG_INFO ("ESTABLISHED -> FIN_WAIT_1");
          m_state = FIN_WAIT_1;
        }
      else if (m_state == CLOSE_WAIT)
        { // On passive close: Peer sent me FIN already
          NS_LOG_INFO ("CLOSE_WAIT -> LAST_ACK");
          m_state = LAST_ACK;
        }
    }
  TcpHeader header;
  header.SetFlags (flags);
  header.SetSequenceNumber (seq);
  header.SetAckNumber (m_rxBuffer.NextRxSequence ());
  if (m_endPoint)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  header.SetWindowSize (AdvertisedWindowSize ());
  AddOptions (header);
  if (m_retxEvent.IsExpired () )
    { // Schedule retransmit
      m_rto = m_rtt->RetransmitTimeout ();
      NS_LOG_LOGIC (this << " SendDataPacket Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto.Get ()).GetSeconds () );
      m_retxEvent = Simulator::Schedule (m_rto, &TcpSocketBase::ReTxTimeout, this);
    }
  NS_LOG_LOGIC ("Send packet via TcpL4Protocol with flags 0x" << std::hex << static_cast<uint32_t> (flags) << std::dec);
  if (m_endPoint)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }
  m_rtt->SentSeq (seq, sz);       // notify the RTT
  // Notify the application of the data being sent unless this is a retransmit
  if (seq == m_nextTxSequence)
    {
      Simulator::ScheduleNow (&TcpSocketBase::NotifyDataSent, this, sz);
    }
  // Update highTxMark
  m_highTxMark = std::max (seq + sz, m_highTxMark.Get ());
  return sz;
}

void TcpRC3Sack::SendEmptyPacket(uint8_t flags){
  SendEmptyPacket(flags, 0, false);
}

/** Send an empty packet with specified TCP flags */
void
TcpRC3Sack::SendEmptyPacket (uint8_t flags, uint64_t priority, bool withSack)
{
  NS_LOG_INFO(this<<" I am here, in empty packet "<<m_flowid<<"\t"<<(uint64_t)priority<<"\t"<<m_rxBuffer.NextRxSequence ());
  NS_LOG_FUNCTION (this << (uint32_t)flags);
  Ptr<Packet> p = Create<Packet> ();
  TcpHeader header;
  SequenceNumber32 s = m_nextTxSequence;

  MyPriorityTag tag;
  tag.SetId(m_flowid);
  tag.SetPriority(priority);
  p -> AddPacketTag(tag);



  /*
   * Add tags for each socket option.
   * Note that currently the socket adds both IPv4 tag and IPv6 tag
   * if both options are set. Once the packet got to layer three, only
   * the corresponding tags will be read.
   */
  if (IsManualIpTos ())
    {    
      SocketIpTosTag ipTosTag;
      ipTosTag.SetTos (GetIpTos ()); 
      p->AddPacketTag (ipTosTag);
    }    

  if (IsManualIpv6Tclass ())
    {    
      SocketIpv6TclassTag ipTclassTag;
      ipTclassTag.SetTclass (GetIpv6Tclass ()); 
      p->AddPacketTag (ipTclassTag);
    }    

  if (IsManualIpTtl ())
    {    
      SocketIpTtlTag ipTtlTag;
      ipTtlTag.SetTtl (GetIpTtl ()); 
      p->AddPacketTag (ipTtlTag);
    }    

  if (IsManualIpv6HopLimit ())
    {    
      SocketIpv6HopLimitTag ipHopLimitTag;
      ipHopLimitTag.SetHopLimit (GetIpv6HopLimit ()); 
      p->AddPacketTag (ipHopLimitTag);
    }    

  if (m_endPoint == 0 && m_endPoint6 == 0)
    {    
      NS_LOG_WARN ("Failed to send empty packet due to null endpoint");
      return;
    }    
  if (flags & TcpHeader::FIN)
    {    
      flags |= TcpHeader::ACK;
    }    
  else if (m_state == FIN_WAIT_1 || m_state == LAST_ACK || m_state == CLOSING)
    {
      ++s;
    }

  header.SetFlags (flags);
  header.SetSequenceNumber (s);
  header.SetAckNumber (m_rxBuffer.NextRxSequence ());
  if (m_endPoint != 0)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  header.SetWindowSize (AdvertisedWindowSize ());

 /****SACK change****/
  if(withSack)
  {
      Ptr<TcpOptionSack> sack_option = CreateObject<TcpOptionSack> ();
      int sz = 3;
      if(m_sackstack.size() < SACKSTACKSZ)
        sz = (int)m_sackstack.size();
      for(int i=0; i < sz; i++)
      {
          SackBlock s = std::make_pair(m_sackstack[i].left, m_sackstack[i].right);
          sack_option->AddSack(s);
      }

      header.AppendOption(sack_option);

      }
  /****************/



  AddOptions (header);
  m_rto = m_rtt->RetransmitTimeout ();
  bool hasSyn = flags & TcpHeader::SYN;
  bool hasFin = flags & TcpHeader::FIN;
  bool isAck = flags == TcpHeader::ACK;
  if (hasSyn)
    {
      if (m_cnCount == 0)
        { // No more connection retries, give up
          NS_LOG_LOGIC ("Connection failed.");
          CloseAndNotify ();
          return;
        }
      else
        { // Exponential backoff of connection time out
          int backoffCount = 0x1 << (m_cnRetries - m_cnCount);
          m_rto = m_cnTimeout * backoffCount;
          m_cnCount--;
        }
    }
  if (m_endPoint != 0)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }

  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }
  if (flags & TcpHeader::ACK)
    { // If sending an ACK, cancel the delay ACK as well
      m_delAckEvent.Cancel ();
      m_delAckCount = 0;
    }
  if (m_retxEvent.IsExpired () && (hasSyn || hasFin) && !isAck )
    { // Retransmit SYN / SYN+ACK / FIN / FIN+ACK to guard against lost
      NS_LOG_LOGIC ("Schedule retransmission timeout at time "
                    << Simulator::Now ().GetSeconds () << " to expire at time "
                    << (Simulator::Now () + m_rto.Get ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, &TcpSocketBase::SendEmptyPacket, this, flags);
    }
}


/****SACK Change****/
//Send Pending Data changed drastically to transmit only un-sacked data
bool
TcpRC3Sack::SendPendingData (bool withAck)
{
  NS_LOG_FUNCTION (this << withAck);
  if (m_txBuffer.Size () == 0)
    {
      return false;                           // Nothing to send

    }
  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_INFO ("TcpRC3Sack::SendPendingData: No endpoint; m_shutdownSend=" << m_shutdownSend);
      return false; // Is this the right way to handle this condition?
    }
  uint32_t nPacketsSent = 0;

  /***SACK CHANGE***/
  uint32_t w = AvailableWindow();
 
  while(w > 0)
  {
      SequenceNumber32 seqNo = m_scoreboard.GetNextAggSegment(m_nextTxSequence);


      if(seqNo == SequenceNumber32(0))
        break;

      if(seqNo >= m_txBuffer.TailSequence())
        break;
      
      NS_LOG_LOGIC("TcpRC3Sack " << this << " SendPendingData" <<
                    " w " << w <<
                    " rxwin " << m_rWnd <<
                    " segsize " << m_segmentSize <<
                    " nextTxSeq " << m_nextTxSequence <<
                    " seqSending " << seqNo <<
                    " highestRxAck " << m_txBuffer.HeadSequence () <<
                    " pd->Size " << m_txBuffer.Size () <<
                    " pd->SFS " << m_txBuffer.SizeFromSequence (m_nextTxSequence));

      // Quit if send disallowed
      if (m_shutdownSend)
        {
          m_errno = ERROR_SHUTDOWN;
          return false;
        }

      // Stop sending if we need to wait for a larger Tx window (prevent silly window syndrome)
      if (w < m_segmentSize && m_txBuffer.SizeFromSequence (seqNo) > w)
        {
          break; // No more
        }
      // Nagle's algorithm (RFC896): Hold off sending if there is unacked data
      // in the buffer and the amount of data to send is less than one segment
      if (!m_noDelay && UnAckDataCount () > 0
          && m_txBuffer.SizeFromSequence (seqNo) < m_segmentSize)
        {
          NS_LOG_LOGIC ("Invoking Nagle's algorithm. Wait to send.");
          break;
        }

      uint32_t s = std::min (w, m_segmentSize);
      uint32_t sz = SendDataPacket (seqNo, s, withAck);

      //updated ReTx in case of retransmitted packet
      if(seqNo < m_nextTxSequence)
        m_scoreboard.m_highReTx = seqNo;


      if(seqNo >= m_nextTxSequence)
      {
        m_nextTxSequence = m_scoreboard.GetNextAggSegment(seqNo + sz);
        if(m_nextTxSequence == SequenceNumber32(0)) //will create a mess, safe option is to increase by sz
            m_nextTxSequence = seqNo + sz;
      }

      nPacketsSent++;

      w = AvailableWindow();


  }

  NS_LOG_LOGIC ("SendPendingData sent " << nPacketsSent << " packets; next sequence " << m_nextTxSequence << ":)");
 
  return (nPacketsSent > 0);
}



void
TcpRC3Sack::StartRLPLoop()
{
  
   for(int i = 0; i < int(m_txBuffer.Size()/m_segmentSize); i++){
      if(!SendLowPriorityPacket())
            break;
   }
}


bool
TcpRC3Sack::SendLowPriorityPacket()
{
  if(m_state != ESTABLISHED) return false;
  
  if(!rc3_p2) return false;
  
  p2_block_max = m_txBuffer.TailSequence().GetValue();


  uint32_t maxSize = m_segmentSize;
 
  SequenceNumber32 seq(p2_block_max - p2_block_bytes_sent - maxSize);
  
  if(seq + maxSize <= m_nextTxSequence)
  {
    NS_LOG_INFO("***Priority 0 took care of all data***");
    p2_block_bytes_sent = 0;
    last_block_seq = p2_block_max;
    return false;
  }

  if(seq.GetValue() < last_block_seq)
  {
    NS_LOG_INFO("***All data already sent***");
    p2_block_bytes_sent = 0;
    last_block_seq = p2_block_max;
    return false;
  }

  p2_block_bytes_sent += maxSize;

  NS_LOG_FUNCTION (this << seq << maxSize );
  Ptr<Packet> p = m_txBuffer.CopyFromSequence (maxSize, seq);
  uint8_t flags =  0; 


  uint64_t priority = 1;

  uint32_t maxseq = p2_block_max;
  int packetCounter;
  packetCounter = (maxseq - seq.GetValue())/maxSize;
  float temp =  ((float(packetCounter)*9)/(float(m_prioritySlots)*10))+1;

  if(m_multipriorities)
      priority = ceil(log10(temp));
  if(m_lstf)
      priority = priority * 10000000000;

  MyPriorityTag tag;
  tag.SetId(m_flowid);
  tag.SetPriority(priority);
  p -> AddPacketTag(tag);

  NS_LOG_INFO(this<<"Sending rc3 data "<<seq<<" "<<packetCounter<<" "<<tag.GetId()<<" "<<(uint64_t)tag.GetPriority());
  if (IsManualIpTos ())
    {    
      SocketIpTosTag ipTosTag;
      ipTosTag.SetTos (GetIpTos ()); 
      p->AddPacketTag (ipTosTag);
    }    

  if (IsManualIpv6Tclass ())
    {    
      SocketIpv6TclassTag ipTclassTag;
      ipTclassTag.SetTclass (GetIpv6Tclass ()); 
      p->AddPacketTag (ipTclassTag);
    }

  if (IsManualIpTtl ())
    {
      SocketIpTtlTag ipTtlTag;
      ipTtlTag.SetTtl (GetIpTtl ());
      p->AddPacketTag (ipTtlTag);
    }

  if (IsManualIpv6HopLimit ())
    {
      SocketIpv6HopLimitTag ipHopLimitTag;
      ipHopLimitTag.SetHopLimit (GetIpv6HopLimit ());
      p->AddPacketTag (ipHopLimitTag);
    }

  TcpHeader header;
  header.SetFlags (flags);
  header.SetSequenceNumber (seq);
  header.SetAckNumber (m_rxBuffer.NextRxSequence ());
  if (m_endPoint)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  header.SetWindowSize (AdvertisedWindowSize ());
  AddOptions (header);
  if (m_endPoint)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }
  // Update highTxMark
  //m_highTxMark = std::max (seq + sz, m_highTxMark.Get ());
  //m_nextTxSequence += sz;
  return true;
}

} // namespace ns3
