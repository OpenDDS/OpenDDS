/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPESUPPORTIMPL_H
#define OPENDDS_DCPS_TYPESUPPORTIMPL_H

#include "dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class MetaStruct;

class OpenDDS_Dcps_Export TypeSupportImpl {
public:

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual ~TypeSupportImpl() {}
  virtual const MetaStruct& getMetaStructForType() = 0;
#endif

};

}
}

#endif
