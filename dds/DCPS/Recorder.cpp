/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Recorder.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RecorderListener::~RecorderListener()
{
}

Recorder::~Recorder()
{
}

XTypes::DynamicData Recorder::get_dynamic_data(const RawDataSample& sample)
{
  Encoding enc(sample.encoding_kind_, sample.header_.byte_order_ ? ENDIAN_LITTLE : ENDIAN_BIG);
  const DynamicTypeByPubId::const_iterator dt_found = dt_map_.find(sample.publication_id_);
  if (dt_found == dt_map_.end()) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: on_sample_data_received: "
        "failed to find GUID: %C in DynamicTypeByPubId.\n", LogGuid(sample.publication_id_).c_str()));
    }
    return XTypes::DynamicData();
  } else {
    XTypes::DynamicType_rch dt = dt_found->second;
    XTypes::DynamicData dd(sample.sample_.get(), enc, dt);
    return dd;
  }
}

Recorder_ptr Recorder::_duplicate(Recorder_ptr obj)
{
  if (obj) obj->_add_ref();
  return obj;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::Recorder_ptr
TAO::Objref_Traits<OpenDDS::DCPS::Recorder>::duplicate(OpenDDS::DCPS::Recorder_ptr p)
{
  return OpenDDS::DCPS::Recorder::_duplicate(p);
}

void
TAO::Objref_Traits<OpenDDS::DCPS::Recorder>::release(OpenDDS::DCPS::Recorder_ptr p)
{
  if (p) p->_remove_ref();
}

OpenDDS::DCPS::Recorder_ptr
TAO::Objref_Traits<OpenDDS::DCPS::Recorder>::nil()
{
  return static_cast<OpenDDS::DCPS::Recorder_ptr>(0);
}

CORBA::Boolean
TAO::Objref_Traits<OpenDDS::DCPS::Recorder>::marshal(const OpenDDS::DCPS::Recorder_ptr,
                                                     TAO_OutputCDR&)
{
  return false;
}

TAO_END_VERSIONED_NAMESPACE_DECL
