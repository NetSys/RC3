#include "ns3/my-priority-tag.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (MyPriorityTag);

TypeId
MyPriorityTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyPriorityTag")
                      .SetParent<Tag> ()
                      .AddConstructor<MyPriorityTag>()
                      ;
  return tid;
}

TypeId
MyPriorityTag::GetInstanceTypeId (void) const
{
  return GetTypeId();
}

MyPriorityTag::MyPriorityTag()
{
  m_flowid = 0;
  m_priority = 0;
  m_timestamp = 0;
  m_seq = 0;
}

void MyPriorityTag::SetId(uint32_t flowid)
{
  m_flowid = flowid;
}
uint32_t MyPriorityTag::GetId() const
{
  return m_flowid;
}

void MyPriorityTag::SetPriority(uint64_t priority)
{
  m_priority = priority;
}
uint64_t MyPriorityTag::GetPriority() const
{
  return m_priority;
}

void MyPriorityTag::SetTimestamp(uint64_t timestamp)
{
  m_timestamp = timestamp;
}
uint64_t MyPriorityTag::GetTimestamp() const
{
  return m_timestamp;
}



uint32_t MyPriorityTag::GetSerializedSize (void) const
{
  return 24;
}

void MyPriorityTag::Serialize (TagBuffer i) const
{
  i.WriteU32(m_flowid);
  i.WriteU64(m_priority);
  i.WriteU64(m_timestamp);
  i.WriteU32(m_seq);
}

void MyPriorityTag::Deserialize (TagBuffer i)
{
  m_flowid = i.ReadU32();
  m_priority = i.ReadU64();
  m_timestamp = i.ReadU64();
  m_seq = i.ReadU32();
}

void MyPriorityTag::Print(std::ostream &os) const
{
  os << "Id: " << (uint32_t)m_flowid;
  os << ", Priority: " << (uint64_t)m_priority;
}

} //namespace ns3

