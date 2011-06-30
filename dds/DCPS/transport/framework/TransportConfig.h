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
#include "TransportInst.h"
#include "TransportInst_rch.h"

#include <ace/Synch_Traits.h>

#include <vector>
#include <string>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportConfig : public RcObject<ACE_SYNCH_MUTEX> {
public:

  std::string name() const { return name_; }

  std::vector<TransportInst_rch> instances_;

private:
  friend class TransportRegistry;
  explicit TransportConfig(const std::string& name);
  ~TransportConfig();

  const std::string name_;
};

}
}

#if defined(__ACE_INLINE__)
#include "TransportConfig.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORTCONFIG_H */
