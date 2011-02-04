/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTIMPLFACTORY_H
#define OPENDDS_DCPS_TRANSPORTIMPLFACTORY_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "TransportImpl.h"
#include "TransportImpl_rch.h"
#include "ace/Synch.h"

namespace OpenDDS {
namespace DCPS {

class TransportImpl;
class TransportReactorTask;

class OpenDDS_Dcps_Export TransportImplFactory : public RcObject<ACE_SYNCH_MUTEX> {
public:

  virtual ~TransportImplFactory();

  TransportImpl* create_impl();
  TransportImpl* create_impl(TransportReactorTask* reactor_task);
//MJM: Why not just have this method with a default null value?

  // Subclass should override if it requires the reactor because
  // the default implementation is that the reactor is not required.
  virtual int requires_reactor() const;

protected:

  TransportImplFactory();

  /// This should return 0 (nil) if the create() cannot be done
  /// for any reason.
  virtual TransportImpl* create() = 0;

};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TransportImplFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTIMPLFACTORY_H */
