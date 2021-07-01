#ifndef OPENDDS_DCPS_LOGADDR_H
#define OPENDDS_DCPS_LOGADDR_H

#include "dcps_export.h"

#include <dds/Versioned_Namespace.h>

#include <ace/INET_Addr.h>

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export LogAddr
{
public:
  enum Fmt {IP, IP_PORT, HOST, HOST_PORT, IP_PORT_HOST};
  explicit LogAddr(const ACE_INET_Addr& addr, Fmt fmt = IP_PORT);
  const std::string& str() const { return addr_; }
  const char* c_str() const { return addr_.c_str(); }
private:
  std::string addr_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_LOGADDR_H
