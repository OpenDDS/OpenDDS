/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ASSOCIATIONDATA_H
#define OPENDDS_DCPS_ASSOCIATIONDATA_H

#include "NetworkResource.h"
#include "transport/framework/TransportDefs.h"

#include <dds/DdsDcpsInfoUtilsC.h>

#include <ace/INET_Addr.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct AssociationData {
  RepoId               remote_id_;
  TransportLocatorSeq  remote_data_;
  TransportLocator     discovery_locator_;
  MonotonicTime_t      participant_discovered_at_;
  ACE_CDR::ULong       remote_transport_context_;
  Priority             publication_transport_priority_;
  bool                 remote_reliable_, remote_durable_;

  AssociationData()
    : remote_id_(GUID_UNKNOWN)
    , discovery_locator_()
    , participant_discovered_at_(monotonic_time_zero())
    , remote_transport_context_(0)
    , publication_transport_priority_(0)
    , remote_reliable_(false)
    , remote_durable_(false)
  {}

  static ACE_INET_Addr get_remote_address(const TransportBLOB& remote)
  {
    ACE_INET_Addr remote_address;
    NetworkResource network_resource;

    // Get the remote address from the "blob" in the remote_info struct.
    ACE_InputCDR cdr((const char*)remote.get_buffer(),
                                  remote.length());

    if ((cdr >> network_resource) == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: AssociationData::get_remote_address")
                 ACE_TEXT(" failed to de-serialize the NetworkResource\n")));
    } else {
      network_resource.to_addr(remote_address);
    }

    return remote_address;
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_ASSOCIATIONDATA_H */
