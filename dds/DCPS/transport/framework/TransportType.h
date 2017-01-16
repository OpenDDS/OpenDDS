/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_TYPE_H
#define OPENDDS_DCPS_TRANSPORT_TYPE_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/transport/framework/TransportInst.h"
#include "dds/DCPS/PoolAllocator.h"

#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class TransportType
 *
 * @brief Base class for concrete transports to provide new objects.
 *
 * Each transport implementation will need to define a concrete
 * subclass of the TransportType class.  The base
 * class (TransportType) contains the pure virtual functions to
 * provide new objects. The concrete transport implements these methods
 * to provide the new concrete transport object.
 *
 */
class OpenDDS_Dcps_Export TransportType : public RcObject<ACE_SYNCH_MUTEX> {
public:

  virtual const char* name() = 0;

  virtual TransportInst_rch new_inst(const OPENDDS_STRING& name) = 0;

protected:

  TransportType();
  virtual ~TransportType();
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
