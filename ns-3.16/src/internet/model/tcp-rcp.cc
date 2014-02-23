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
 * Author: Radhika Mittal, built upon code by Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }

#include "tcp-rcp.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-route.h"
#include "tcp-l4-protocol.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include "ipv6-l3-protocol.h"
#include "ns3/rcp-tag.h"



#define INFTIME 10

NS_LOG_COMPONENT_DEFINE ("TcpRCP");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpRCP);

TypeId
TcpRCP::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpRCP")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpRCP> ()
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&TcpRCP::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LimitedTransmit", "Enable limited transmit",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpRCP::m_limitedTx),
		    MakeBooleanChecker ())
    .AddAttribute ("InitBottleneckRate", "Initial Bottleneck Rate set to link rate",
		    DoubleValue (125000000),
		    MakeDoubleAccessor (&TcpRCP::m_bottleneckRate),
		    MakeDoubleChecker<double> ())
    .AddAttribute ("FlowId", "Flow id",
                    UintegerValue (0),
                    MakeUintegerAccessor (&TcpRCP::m_flowid),
                    MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpRCP::m_cWnd))
  ;
  return tid;
}

TcpRCP::TcpRCP (void)
  : m_retxThresh (3), // mute valgrind, actual value set by the attribute system
    m_inFastRec (false),
    m_limitedTx (false), // mute valgrind, actual value set by the attribute system
    m_flowid (0),
    m_bottleneckRate (125000000),
    m_reverseBottleneckRate(-1),
    m_rcprtt(0), //copied from RCP ns-2 code, didn't understand the logic
    m_reverseTs(0),
    m_seenTs(-1),
    m_interval(0),
    m_lastpkttime(0),
    m_rcpRunning(false),
    m_sendEvent()
{
  NS_LOG_FUNCTION (this);
}

TcpRCP::TcpRCP (const TcpRCP& sock)
  : TcpSocketBase (sock),
    m_cWnd (sock.m_cWnd),
    m_ssThresh (sock.m_ssThresh),
    m_initialCWnd (sock.m_initialCWnd),
    m_retxThresh (sock.m_retxThresh),
    m_inFastRec (false),
    m_limitedTx (sock.m_limitedTx),
    m_flowid (sock.m_flowid),
    m_bottleneckRate (sock.m_bottleneckRate),
    m_reverseBottleneckRate(sock.m_reverseBottleneckRate),
    m_rcprtt(sock.m_rcprtt), //copied from RCP ns-2 code, didn't understand the logic
    m_reverseTs(sock.m_reverseTs),
    m_seenTs(sock.m_seenTs),
    m_interval(sock.m_interval),
    m_lastpkttime(sock.m_lastpkttime),
    m_rcpRunning(sock.m_rcpRunning),
    m_sendEvent()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpRCP::~TcpRCP (void)
{
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpRCP::Listen (void)
{
  NS_LOG_FUNCTION (this);
  InitializeCwnd ();
  return TcpSocketBase::Listen ();
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpRCP::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  InitializeCwnd ();
  return TcpSocketBase::Connect (address);
}

/** Limit the size of in-flight data by cwnd and receiver's rxwin */
uint32_t
TcpRCP::Window (void)
{
  NS_LOG_FUNCTION (this);
  return m_cWnd.Get();
}

Ptr<TcpSocketBase>
TcpRCP::Fork (void)
{
  return CopyObject<TcpRCP> (this);
}

/** New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
void
TcpRCP::NewAck (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("TcpRCP receieved ACK for seq " << seq <<
                " cwnd " << m_cWnd <<
                " ssthresh " << m_ssThresh);

  // Check for exit condition of fast recovery
  if (m_inFastRec && seq < m_recover)
    { // Partial ACK, partial window deflation (RFC2582 sec.3 bullet #5 paragraph 3)
      //m_cWnd -= seq - m_txBuffer.HeadSequence ();
      //m_cWnd += m_segmentSize;  // increase cwnd
      NS_LOG_INFO ("Partial ACK in fast recovery: cwnd set to " << m_cWnd);
      TcpSocketBase::NewAck (seq); // update m_nextTxSequence and send new data if allowed by window
      DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
      return;
    }
  else if (m_inFastRec && seq >= m_recover)
    { // Full ACK (RFC2582 sec.3 bullet #5 paragraph 2, option 1)
      //m_cWnd = std::min (m_ssThresh, BytesInFlight () + m_segmentSize);
      m_inFastRec = false;
      NS_LOG_INFO ("Received full ACK. Leaving fast recovery with cwnd set to " << m_cWnd);
    }

  // Increase of cwnd based on current phase (slow start or congestion avoidance)
  /*if (m_cWnd < m_ssThresh)
    { // Slow start mode, add one segSize to cWnd. Default m_ssThresh is 65535. (RFC2001, sec.1)
      m_cWnd += m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }
  else
    { // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
      // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
      double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ();
      adder = std::max (1.0, adder);
      m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }*/

  // Complete newAck processing
  TcpSocketBase::NewAck (seq);
}

/** Cut cwnd and enter fast recovery mode upon triple dupack */
void
TcpRCP::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  if (count == m_retxThresh && !m_inFastRec)
    { // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
      //m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
      //m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_recover = m_highTxMark;
      m_inFastRec = true;
      NS_LOG_INFO ("Triple dupack. Enter fast recovery mode. Reset cwnd to " << m_cWnd <<
                   ", ssthresh to " << m_ssThresh << " at fast recovery seqnum " << m_recover);
      DoRetransmit ();
    }
  else if (m_inFastRec)
    { // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
      //m_cWnd += m_segmentSize;
      NS_LOG_INFO ("Dupack in fast recovery mode. Increase cwnd to " << m_cWnd);
      SendPendingData (m_connected);
    }
  else if (!m_inFastRec && m_limitedTx && m_txBuffer.SizeFromSequence (m_nextTxSequence) > 0)
    { // RFC3042 Limited transmit: Send a new packet for each duplicated ACK before fast retransmit
      NS_LOG_INFO ("Limited transmit");
      //uint32_t sz = SendDataPacket (m_nextTxSequence, m_segmentSize, true);
      //m_nextTxSequence += sz;                    // Advance next tx sequence
    };
}

/** Retransmit timeout */
void
TcpRCP::Retransmit (void)
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
  //m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
  //m_cWnd = m_segmentSize;
  m_nextTxSequence = m_txBuffer.HeadSequence (); // Restart from highest Ack
  NS_LOG_INFO ("RTO. Reset cwnd to " << m_cWnd <<
               ", ssthresh to " << m_ssThresh << ", restart from seqnum " << m_nextTxSequence);
  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  DoRetransmit ();                          // Retransmit the packet
}

void
TcpRCP::SetSegSize (uint32_t size)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpRCP::SetSegSize() cannot change segment size after connection started.");
  m_segmentSize = size;
}

void
TcpRCP::SetSSThresh (uint32_t threshold)
{
  m_ssThresh = threshold;
}

uint32_t
TcpRCP::GetSSThresh (void) const
{
  return m_ssThresh;
}

void
TcpRCP::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpRCP::SetInitialCwnd() cannot change initial cwnd after connection started.");
  m_initialCWnd = cwnd;
}

uint32_t
TcpRCP::GetInitialCwnd (void) const
{
  return m_initialCWnd;
}

void 
TcpRCP::InitializeCwnd (void)
{
  /*
   * Initialize congestion window, default to 1 MSS (RFC2001, sec.1) and must
   * not be larger than 2 MSS (RFC2581, sec.3.1). Both m_initiaCWnd and
   * m_segmentSize are set by the attribute system in ns3::TcpSocket.
   */
  m_cWnd = m_initialCWnd * m_segmentSize;
}


/** The real function to handle the incoming packet from lower layers. This is
    wrapped by ForwardUp() so that this function can be overloaded by daughter
    classes. */
void
TcpRCP::DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                            Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_LOGIC ("Socket " << this << " forward up " <<
                m_endPoint->GetPeerAddress () <<
                ":" << m_endPoint->GetPeerPort () <<
                " to " << m_endPoint->GetLocalAddress () <<
                ":" << m_endPoint->GetLocalPort ());
  Address fromAddress = InetSocketAddress (header.GetSource (), port);
  Address toAddress = InetSocketAddress (header.GetDestination (), m_endPoint->GetLocalPort ());

  //Peel off RCP tag and update rate and data to send
  RCPTag tag; 
  bool peeked = packet->PeekPacketTag(tag);
  NS_ASSERT(peeked);
  NS_ASSERT(tag.GetTypeId().GetName() == "ns3::RCPTag"); 

  NS_LOG_INFO("Received Tag:");

  //updating RTT estimate
  if(tag.GetReverseTs() > m_seenTs)  //this check ignores the reverse timestamp returned by subsequent paced packets
  {
    m_seenTs = tag.GetReverseTs();
    m_rcprtt = Simulator::Now().GetSeconds() - tag.GetReverseTs();
  }

  double newInterval;
  //updating bottleneck rate, cwnd and interval
  if(tag.GetReverseBottleneckRate() >= 0)
  {
      m_bottleneckRate = tag.GetReverseBottleneckRate();
  }
  m_cWnd = (m_bottleneckRate * m_rcprtt)/(m_segmentSize + 40);
  m_cWnd = m_cWnd * m_segmentSize;
  if(m_cWnd < m_segmentSize)
      m_cWnd = m_segmentSize;
  if(m_bottleneckRate > 0)
      newInterval = (m_segmentSize+40) / m_bottleneckRate;
  else
      newInterval = INFTIME; 
  if(newInterval != m_interval)
  {
      m_interval = newInterval;
      RateChange();
  }

  //updating reverse path info
  m_reverseBottleneckRate = tag.GetBottleneckRate();
  m_reverseTs = tag.GetTs();
  NS_LOG_INFO("Current data: Bottleneck Rate: "<<m_bottleneckRate<<", Reverse Bottleneck Rate: "<<m_reverseBottleneckRate<<", reverseTS: "<<m_reverseTs<<" ,Congestion Window: "<<m_cWnd<<" ,RTT: "<<m_rcprtt<<" ,Interval: "<<m_interval);







  // Peel off TCP header and do validity checking
  TcpHeader tcpHeader;
  packet->RemoveHeader (tcpHeader);
  if (tcpHeader.GetFlags () & TcpHeader::ACK)
    {
      EstimateRtt (tcpHeader);
    }
  ReadOptions (tcpHeader);

  // Update Rx window size, i.e. the flow control window
  if (m_rWnd.Get () == 0 && tcpHeader.GetWindowSize () != 0)
    { // persist probes end
      NS_LOG_LOGIC (this << " Leaving zerowindow persist state");
      m_persistEvent.Cancel ();
    }
  m_rWnd = tcpHeader.GetWindowSize ();
  //m_rWnd = 0x3fffffff; //RFC 1323 is enabled by default on all modern OSes JUSTINE
 
  // Discard fully out of range data packets
  if (packet->GetSize ()
      && OutOfRange (tcpHeader.GetSequenceNumber (), tcpHeader.GetSequenceNumber () + packet->GetSize ()))
    {
      NS_LOG_LOGIC ("At state " << TcpStateName[m_state] <<
                    " received packet of seq [" << tcpHeader.GetSequenceNumber () <<
                    ":" << tcpHeader.GetSequenceNumber () + packet->GetSize () <<
                    ") out of range [" << m_rxBuffer.NextRxSequence () << ":" <<
                    m_rxBuffer.MaxRxSequence () << ")");
      // Acknowledgement should be sent for all unacceptable packets (RFC793, p.69)
      if (m_state == ESTABLISHED && !(tcpHeader.GetFlags () & TcpHeader::RST))
        {
          SendEmptyPacket (TcpHeader::ACK);
        }
      return;
    }

  // TCP state machine code in different process functions
  // C.f.: tcp_rcv_state_process() in tcp_input.c in Linux kernel
  switch (m_state)
    {
    case ESTABLISHED:
      ProcessEstablished (packet, tcpHeader);
      break;
    case LISTEN:
      ProcessListen (packet, tcpHeader, fromAddress, toAddress);
      break;
    case TIME_WAIT:
      // Do nothing
      break;
    case CLOSED:
      // Send RST if the incoming packet is not a RST
      if ((tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG)) != TcpHeader::RST)
        { // Since m_endPoint is not configured yet, we cannot use SendRST here
          TcpHeader h;
          h.SetFlags (TcpHeader::RST);
          h.SetSequenceNumber (m_nextTxSequence);
          h.SetAckNumber (m_rxBuffer.NextRxSequence ());
          h.SetSourcePort (tcpHeader.GetDestinationPort ());
          h.SetDestinationPort (tcpHeader.GetSourcePort ());
          h.SetWindowSize (AdvertisedWindowSize ());
          AddOptions (h);
          m_tcp->SendPacket (Create<Packet> (), h, header.GetDestination (), header.GetSource (), m_boundnetdevice);
        }
      break;
    case SYN_SENT:
      ProcessSynSent (packet, tcpHeader);
      break;
    case SYN_RCVD:
      ProcessSynRcvd (packet, tcpHeader, fromAddress, toAddress);
      break;
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case CLOSE_WAIT:
      ProcessWait (packet, tcpHeader);
      break;
    case CLOSING:
      ProcessClosing (packet, tcpHeader);
      break;
    case LAST_ACK:
      ProcessLastAck (packet, tcpHeader);
      break;
    default: // mute compiler
      break;
    }
}

/** Send an empty packet with specified TCP flags */
void
TcpRCP::SendEmptyPacket (uint8_t flags)
{
  NS_LOG_FUNCTION (this << (uint32_t)flags);
  Ptr<Packet> p = Create<Packet> ();
  TcpHeader header;
  SequenceNumber32 s = m_nextTxSequence;

  //Adding the RCP tag
  NS_LOG_INFO("Current data: Bottleneck Rate: "<<m_bottleneckRate<<", Reverse Bottleneck Rate: "<<m_reverseBottleneckRate<<", reverseTS: "<<m_reverseTs<<" ,Congestion Window: "<<m_cWnd<<" ,RTT: "<<m_rcprtt<<" ,Interval: "<<m_interval);
  RCPTag tag;
  tag.SetId(m_flowid);
  tag.SetBottleneckRate(-1); //requested rate is -1 (change it later though)
  tag.SetReverseBottleneckRate(m_reverseBottleneckRate); 
  tag.SetRTT(0); 
  tag.SetTs(Simulator::Now().GetSeconds()); 
  tag.SetReverseTs(m_reverseTs); 
  p -> AddPacketTag(tag);
  NS_LOG_INFO("Set tag:");

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


/** Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
    TCP header, and send to TcpL4Protocol */
uint32_t
TcpRCP::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{
  NS_LOG_FUNCTION (this << seq << maxSize << withAck);

  Ptr<Packet> p = m_txBuffer.CopyFromSequence (maxSize, seq);
  uint32_t sz = p->GetSize (); // Size of packet
  uint8_t flags = withAck ? TcpHeader::ACK : 0;
  uint32_t remainingData = m_txBuffer.SizeFromSequence (seq + SequenceNumber32 (sz));

  //Adding the RCP tag
  RCPTag tag;
  tag.SetId(m_flowid);
  tag.SetBottleneckRate(-1); //requested rate is -1 (change it later though)
  tag.SetReverseBottleneckRate(m_reverseBottleneckRate); 
  tag.SetRTT(m_rcprtt); 
  tag.SetTs(Simulator::Now().GetSeconds()); 
  tag.SetReverseTs(m_reverseTs); 
  p -> AddPacketTag(tag);
  NS_LOG_INFO("Set tag for packet number:"<<(seq.GetValue()-1)/1460);




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



/** Send as much pending data as possible according to the Tx window. Note that
 *  this function did not implement the PSH flag
 */
bool
TcpRCP::SendPendingData (bool withAck)
{
  //std::cout << "LAKALAKA?" << std::endl;
  NS_LOG_FUNCTION (this << withAck);

  if(m_rcpRunning)
  {
    NS_LOG_INFO("RCP pacing has already started, data will be sent in time");
    return true;
  }

  else
  {
    m_rcpRunning = true;
    SendNextPacket();
    return true;
  }

  return false;

 }


void
TcpRCP::SendNextPacket()
{

  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_INFO ("TcpRcp::SendPendingData: No endpoint; m_shutdownSend=" << m_shutdownSend);
    }
  uint32_t w = AvailableWindow (); // Get available window size
  uint32_t nPacketsSent = 0;                             // Count sent this loop
  if ((m_txBuffer.Size () > 0) && (m_endPoint !=0 ) && (w >= m_segmentSize))
  {
      NS_LOG_LOGIC("TcpRCP " << this << " SendNextData" <<
                    " w " << w <<
                    " rxwin " << m_rWnd <<
                    " segsize " << m_segmentSize <<
                    " nextTxSeq " << m_nextTxSequence <<
                    " highestRxAck " << m_txBuffer.HeadSequence () <<
                    " pd->Size " << m_txBuffer.Size () <<
                    " pd->SFS " << m_txBuffer.SizeFromSequence (m_nextTxSequence));
      // Quit if send disallowed
      if (m_shutdownSend)
        {
          m_errno = ERROR_SHUTDOWN;
          m_rcpRunning = false;
          return;
        }
      if (m_txBuffer.SizeFromSequence (m_nextTxSequence) < m_segmentSize)
      {    
          NS_LOG_LOGIC ("No data to send currenty");
          m_rcpRunning = false;
          return;
      } 
      
       
      uint32_t s = std::min (w, m_segmentSize);  // Send no more than window
      uint32_t sz = SendDataPacket (m_nextTxSequence, s, true); //Send data with ack
      
      m_lastpkttime = Simulator::Now().GetSeconds();
      nPacketsSent++;                             // Count sent this loop
      m_nextTxSequence += sz;                     // Advance next tx sequence
      
  }
  else
  {
    m_rcpRunning = false;
    return;
  }

  NS_LOG_LOGIC ("SendNextData sent "<<nPacketsSent<<" packets, next sequence " << m_nextTxSequence << ":)");

  m_sendEvent = Simulator::Schedule(Seconds(m_interval), &TcpRCP::SendNextPacket, this);

}

void
TcpRCP::RateChange()
{
  if(!m_rcpRunning)
    return;
 
  NS_LOG_INFO("Changing Rate");
  m_sendEvent.Cancel();

  double t = m_lastpkttime + m_interval;
  double now = Simulator::Now().GetSeconds();

  if(t > now)
  {
      NS_LOG_INFO("t = "<<t<<", now = "<<now<<", Scheduling after: " << t-now);
      double nextSchedule = t - now;
      m_sendEvent = Simulator::Schedule(Seconds(nextSchedule), &TcpRCP::SendNextPacket, this);  
  }
  else
      SendNextPacket();
}







} // namespace ns3
