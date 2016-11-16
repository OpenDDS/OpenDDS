/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPLOADER_H
#define DCPS_RTPSUDPLOADER_H

#include "Rtps_Udp_Export.h"

#include "ace/Global_Macros.h"
#include "ace/Service_Config.h"
#include "ace/Service_Object.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Rtps_Udp_Export RtpsUdpLoader
  : public ACE_Service_Object {
public:
  virtual int init(int argc, ACE_TCHAR* argv[]);
};

ACE_STATIC_SVC_DECLARE_EXPORT(OpenDDS_Rtps_Udp, RtpsUdpLoader)
ACE_FACTORY_DECLARE(OpenDDS_Rtps_Udp, RtpsUdpLoader)

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RTPSUDPLOADER_H */
