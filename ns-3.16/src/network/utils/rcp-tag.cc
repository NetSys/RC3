#include "ns3/rcp-tag.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (RCPTag);

TypeId
RCPTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RCPTag")
                      .SetParent<Tag> ()
                      .AddConstructor<RCPTag>()
                      ;
  return tid;
}

TypeId
RCPTag::GetInstanceTypeId (void) const
{
  return GetTypeId();
}

RCPTag::RCPTag()
{
  m_flowid = 0;
  m_bottleneckRate = 0;
  m_reverseBottleneckRate = 0;
  m_rtt = 0;
  m_ts = 0;
  m_reverseTs = 0;
}

void RCPTag::SetId(uint32_t flowid)
{
  m_flowid = flowid;
}
uint32_t RCPTag::GetId() const
{
  return m_flowid;
}

void RCPTag::SetBottleneckRate(double bottleneckRate)
{
  m_bottleneckRate = bottleneckRate;
}
double RCPTag::GetBottleneckRate() const
{
  return m_bottleneckRate;
}

void RCPTag::SetReverseBottleneckRate(double reverseBottleneckRate)
{
  m_reverseBottleneckRate = reverseBottleneckRate;
}
double RCPTag::GetReverseBottleneckRate() const
{
  return m_reverseBottleneckRate;
}

void RCPTag::SetRTT(double rtt)
{
  m_rtt = rtt;
}
double RCPTag::GetRTT() const
{
  return m_rtt;
}

void RCPTag::SetTs(double ts)
{
  m_ts = ts;
}
double RCPTag::GetTs() const
{
  return m_ts;
}

void RCPTag::SetReverseTs(double reverseTs)
{
  m_reverseTs = reverseTs;
}
double RCPTag::GetReverseTs() const
{
  return m_reverseTs;
}

uint32_t RCPTag::GetSerializedSize (void) const
{
  uint32_t size = sizeof(uint32_t) + 5*sizeof(double);
  return size;
}

void RCPTag::Serialize (TagBuffer i) const
{
  i.WriteU32(m_flowid);
  i.WriteDouble(m_bottleneckRate);
  i.WriteDouble(m_reverseBottleneckRate);
  i.WriteDouble(m_rtt);
  i.WriteDouble(m_ts);
  i.WriteDouble(m_reverseTs);
}

void RCPTag::Deserialize (TagBuffer i)
{
  m_flowid = i.ReadU32();
  m_bottleneckRate = i.ReadDouble();
  m_reverseBottleneckRate = i.ReadDouble();
  m_rtt = i.ReadDouble();
  m_ts = i.ReadDouble();
  m_reverseTs = i.ReadDouble();
}

void RCPTag::Print(std::ostream &os) const
{
  os << "Id: " << (uint32_t)m_flowid;
  os << ", Bottleneck Rate: " << m_bottleneckRate;
  os << ", Reverse Bottleneck Rate: " << m_reverseBottleneckRate;
  os << ", RTT: " << m_rtt;
  os << ", TimeStamp: " << m_ts;
  os << ", Reverse TimeStamp: " << m_reverseTs;
  os << "\n";
}

} //namespace ns3

