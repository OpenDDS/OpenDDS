/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTCONFIG_H
#define OPENDDS_DCPS_TRANSPORTCONFIG_H

#include <ace/config.h>
#ifndef ACE_LACKS_PRAGMA_ONCE
#pragma once
#endif

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "TransportInst_rch.h"

#include <vector>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportConfig : public RcObject<ACE_SYNCH_MUTEX> {
public:
  std::vector<TransportInst_rch> instances_;

private:
  friend class TransportRegistry;
  TransportConfig();
  ~TransportConfig();

};

}
}

#if defined(__ACE_INLINE__)
#include "TransportConfig.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORTCONFIG_H */
