/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_TYPE_H
#define OPENDDS_DCPS_TRANSPORT_TYPE_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/transport/framework/TransportImplFactory.h"
#include "dds/DCPS/transport/framework/TransportInst.h"

#include "ace/Synch.h"
#include <string>

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

  virtual TransportInst* new_inst(const std::string& name) = 0;

  // Subclass should override if it requires the reactor because
  // the default implementation is that the reactor is not required.
  virtual bool requires_reactor() { return false; }

protected:

  TransportType();
  virtual ~TransportType();
};

} // namespace DCPS
} // namespace OpenDDS

#endif
