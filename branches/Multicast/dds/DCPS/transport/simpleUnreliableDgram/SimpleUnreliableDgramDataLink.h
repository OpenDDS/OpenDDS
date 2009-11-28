/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMDATALINK_H
#define OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMDATALINK_H

#include "SimpleUnreliableDgram_export.h"
#include "dds/DCPS/transport/framework/DataLink.h"
#include "ace/INET_Addr.h"
#include "tao/Basic_Types.h"

namespace OpenDDS {
namespace DCPS {

class TransportSendStrategy;
class TransportImpl;

class SimpleUnreliableDgramDataLink : public DataLink {
public:

  SimpleUnreliableDgramDataLink(
    const ACE_INET_Addr& remote_address,
    TransportImpl*       transport_impl,
    CORBA::Long          priority = 0);

  virtual ~SimpleUnreliableDgramDataLink();

  /// Accessor for the remote address.
  const ACE_INET_Addr& remote_address() const;

  int connect(TransportSendStrategy* send_strategy);

protected:

  /// Called when the DataLink is self-releasing because all of its
  /// reservations have been released, or when the TransportImpl is
  /// handling a shutdown() call.
  virtual void stop_i();

private:
  ACE_INET_Addr remote_address_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramDataLink.inl"
#endif /* __ACE_INLINE__ */

#endif
