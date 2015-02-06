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
#include "dds/DCPS/Definitions.h"
#include "dds/DdsDcpsTypeSupportExtC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class MetaStruct;

class OpenDDS_Dcps_Export TypeSupportImpl
  : public virtual LocalObject<TypeSupport> {
public:

  virtual ~TypeSupportImpl();

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual const MetaStruct& getMetaStructForType() = 0;
#endif

  virtual DDS::ReturnCode_t register_type(DDS::DomainParticipant_ptr participant,
                                          const char* type_name);

  virtual char* get_type_name();

private:
  CORBA::String_var type_name_;
};

}
}

#endif
