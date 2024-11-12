/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ENDPOINTCALLBACKS_H
#define OPENDDS_DCPS_ENDPOINTCALLBACKS_H

#include "Definitions.h"
#include "RcObject.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

  namespace ICE {
    class Endpoint;
  }

  namespace XTypes {
    struct TypeInformation;
  }

  namespace DCPS {
    struct IncompatibleQosStatus;
    struct GUID_t;
    class TransportLocatorSeq;

    class OpenDDS_Dcps_Export EndpointCallbacks
      : public virtual RcObject {
    public:

      virtual void update_incompatible_qos(const IncompatibleQosStatus& status) = 0;

      virtual void update_locators(const GUID_t& /*remote*/,
                                   const TransportLocatorSeq& /*locators*/) {}

      virtual void get_flexible_types(const char* /*key*/,
                                      XTypes::TypeInformation& /*type_info*/) {}

      virtual DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint() = 0;
    };

  }
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
