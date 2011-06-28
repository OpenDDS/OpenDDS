/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_GENERATOR_H
#define OPENDDS_DCPS_TRANSPORT_GENERATOR_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/transport/framework/TransportImplFactory.h"
#include "dds/DCPS/transport/framework/TransportInst.h"

#include "ace/Synch.h"

namespace OpenDDS {
namespace DCPS {

/**
 * @class TransportGenerator
 *
 * @brief Base class for concrete transports to provide new objects.
 *
 * Each transport implementation will need to define a concrete
 * subclass of the TransportGenerator class.  The base
 * class (TransportGenerator) contains the pure virtual functions to
 * provide new objects. The concrete transport implements these methods
 * to provide the new concrete transport object.
 *
 * The TransportGenerator object is registered with the Transport6supplied to the
 * TransportImpl::configure() method.
 */
class OpenDDS_Dcps_Export TransportGenerator : public RcObject<ACE_SYNCH_MUTEX> {
public:

  /// Dtor
  virtual ~TransportGenerator();

  virtual TransportImplFactory* new_factory() = 0;

  virtual TransportInst* new_configuration(const TransportIdType id) = 0;

  virtual void default_transport_ids(TransportIdList & ids) = 0;

protected:

  /// Default ctor.
  TransportGenerator();
};

} // namespace DCPS
} // namespace OpenDDS

#endif
