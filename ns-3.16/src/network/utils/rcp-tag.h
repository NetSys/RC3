#ifndef ECN_PRIORITY_TAG_H
#define ECN_PRIORITY_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3
{

class RCPTag : public Tag
{

public:
  RCPTag();

  void SetId(uint32_t id);
  uint32_t GetId() const;

  void SetBottleneckRate(double bottleneckRate);
  double GetBottleneckRate() const;

  void SetReverseBottleneckRate(double reverseBottleneckRate);
  double GetReverseBottleneckRate() const;
  
  void SetRTT(double rtt);
  double GetRTT() const;
  
  void SetTs(double ts);
  double GetTs() const;
  
  void SetReverseTs(double reverseTs);
  double GetReverseTs() const;
  
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize () const;
  virtual void Print (std::ostream &os) const;

private:
  uint32_t m_flowid;
  double m_bottleneckRate;
  double m_reverseBottleneckRate;
  double m_rtt;
  double m_ts;
  double m_reverseTs;
};

} //namespace ns3

#endif /* MY_PRIORITY_TAG_H */
