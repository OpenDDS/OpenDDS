/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLEMCASTTRANSPORT_H
#define OPENDDS_DCPS_SIMPLEMCASTTRANSPORT_H

#include "SimpleUnreliableDgram_export.h"
#include "SimpleUnreliableDgramTransport.h"
#include <string>

namespace OpenDDS {
namespace DCPS {

class SimpleUnreliableDgram_Export SimpleMcastTransport : public SimpleUnreliableDgramTransport {
public:

  SimpleMcastTransport();
  virtual ~SimpleMcastTransport();

protected:

  virtual int configure_socket(TransportConfiguration* config);

  virtual int connection_info_i
  (TransportInterfaceInfo& local_info) const;

  virtual void deliver_sample(ReceivedDataSample&  sample,
                              const ACE_INET_Addr& remote_address,
                              CORBA::Long          priority);

private:

  ACE_INET_Addr local_address_;
  ACE_TString   multicast_group_address_str_;
  ACE_INET_Addr multicast_group_address_;
  bool receiver_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleMcastTransport.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_SIMPLEMCASTTRANSPORT_H */
