/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTSTRATEGY
#define OPENDDS_DCPS_TRANSPORTSTRATEGY

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * This class is the interface through which DataLink uses
 * TransportReceiveStrategy.
 */
class OpenDDS_Dcps_Export TransportStrategy
  : public RcObject<ACE_SYNCH_MUTEX> {
public:

  virtual ~TransportStrategy();

  virtual int start() = 0;
  virtual void stop() = 0;
};

} // namespace DCPS */
} // namespace OpenDDS */

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORTSTRATEGY */
