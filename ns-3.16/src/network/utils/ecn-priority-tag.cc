#include "ns3/ecn-priority-tag.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (EcnPriorityTag);

TypeId
EcnPriorityTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EcnPriorityTag")
                      .SetParent<Tag> ()
                      .AddConstructor<EcnPriorityTag>()
                      ;
  return tid;
}

TypeId
EcnPriorityTag::GetInstanceTypeId (void) const
{
  return GetTypeId();
}

EcnPriorityTag::EcnPriorityTag()
{
  m_ecn = 1;
  m_priority = 0;
  m_ecnEcho = 0;
}

void EcnPriorityTag::SetEcn(uint8_t value)
{
  m_ecn = value;
}
uint8_t EcnPriorityTag::GetEcn() const
{
  return m_ecn;
}

void EcnPriorityTag::SetPriority(uint8_t priority)
{
  m_priority = priority;
}
uint8_t EcnPriorityTag::GetPriority() const
{
  return m_priority;
}

void EcnPriorityTag::SetEcnEcho(uint8_t ecnEcho)
{
  m_ecnEcho = ecnEcho;
}
uint8_t EcnPriorityTag::GetEcnEcho() const
{
  return m_ecnEcho;
}

uint32_t EcnPriorityTag::GetSerializedSize (void) const
{
  return 3;
}

void EcnPriorityTag::Serialize (TagBuffer i) const
{
  i.WriteU8(m_ecn);
  i.WriteU8(m_priority);
  i.WriteU8(m_ecnEcho);
}

void EcnPriorityTag::Deserialize (TagBuffer i)
{
  m_ecn = i.ReadU8();
  m_priority = i.ReadU8();
  m_ecnEcho = i.ReadU8();
}

void EcnPriorityTag::Print(std::ostream &os) const
{
  os << "Ecn: " << m_ecn;
  os << ", Priority: " << m_priority;
  os << ", EcnEcho: " << m_ecnEcho;
}

} //namespace ns3

