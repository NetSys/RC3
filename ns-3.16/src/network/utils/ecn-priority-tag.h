#ifndef ECN_PRIORITY_TAG_H
#define ECN_PRIORITY_TAG_H

#include "ns3/tag.h"

namespace ns3
{

class EcnPriorityTag : public Tag
{

public:
  EcnPriorityTag();

  void SetEcn(uint8_t ecn);
  uint8_t GetEcn() const;

  void SetPriority(uint8_t priority);
  uint8_t GetPriority() const;

  void SetEcnEcho(uint8_t ecnEcho);
  uint8_t GetEcnEcho() const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;

private:
  uint8_t m_ecn;
  uint8_t m_priority;
  uint8_t m_ecnEcho;
};

} //namespace ns3

#endif /* ECN_PRIORITY_TAG_H */
