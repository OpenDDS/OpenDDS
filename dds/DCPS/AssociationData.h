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
  TransportLocator     remote_data_;
  ACE_INET_Addr        remote_addess_;
  NetworkAddress       network_order_address_;
  CORBA::Long          publication_transport_priority_;

  ACE_INET_Addr& get_remote_address()
  {
    if (this->remote_addess_ == ACE_INET_Addr()) {
      // Get the remote address from the "blob" in the remote_info struct.
      ACE_InputCDR cdr((const char*)remote_data_.data.get_buffer(), remote_data_.data.length());

      if (cdr >> this->network_order_address_ == 0) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: AssociationData::get_remote_address failed "
                  "to de-serialize the NetworkAddress\n"));
      }
      else {
        this->network_order_address_.to_addr(remote_addess_);
      }
    }

    return this->remote_addess_;
  }
};

typedef std::vector<AssociationData> AssociationDataList;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_DCPS_ASSOCIATIONDATA_H */
