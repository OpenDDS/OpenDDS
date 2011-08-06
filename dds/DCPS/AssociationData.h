/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ASSOCIATIONDATA_H
#define OPENDDS_DCPS_ASSOCIATIONDATA_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "ace/INET_Addr.h"

#include <vector>

namespace OpenDDS {
namespace DCPS {

struct AssociationData {
  RepoId               remote_id_;
  TransportLocatorSeq  remote_data_;
  CORBA::Long          publication_transport_priority_;

  static ACE_INET_Addr get_remote_address(const TransportBLOB& remote)
  {
    ACE_INET_Addr remote_address;
    NetworkAddress network_order_address;

    // Get the remote address from the "blob" in the remote_info struct.
    ACE_InputCDR cdr((const char*)remote.get_buffer(),
                                  remote.length());

    if (cdr >> network_order_address == 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: AssociationData::get_remote_address")
                 ACE_TEXT(" failed to de-serialize the NetworkAddress\n")));
    } else {
      network_order_address.to_addr(remote_address);
    }

    return remote_address;
  }
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_DCPS_ASSOCIATIONDATA_H */
