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

void MyPriorityTag::SetPriority(uint8_t priority)
{
  m_priority = priority;
}
uint8_t MyPriorityTag::GetPriority() const
{
  return m_priority;
}

uint32_t MyPriorityTag::GetSerializedSize (void) const
{
  return 9;
}

void MyPriorityTag::Serialize (TagBuffer i) const
{
  i.WriteU32(m_flowid);
  i.WriteU8(m_priority);
  i.WriteU32(m_seq);
}

void MyPriorityTag::Deserialize (TagBuffer i)
{
  m_flowid = i.ReadU32();
  m_priority = i.ReadU8();
  m_seq = i.ReadU32();
}

void MyPriorityTag::Print(std::ostream &os) const
{
  os << "Id: " << (uint32_t)m_flowid;
  os << ", Priority: " << (uint16_t)m_priority;
}

} //namespace ns3

