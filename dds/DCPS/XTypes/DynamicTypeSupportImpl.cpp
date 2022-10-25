/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE

#  include "DynamicTypeSupportImpl.h"

#  include "Utils.h"

#  include <dds/DCPS/debug.h>
#  include <dds/DCPS/DCPS_Utils.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

bool DynamicSample::serialize(DCPS::Serializer& ser) const
{
  ACE_UNUSED_ARG(ser);
  // TODO
  return false;
}

bool DynamicSample::deserialize(DCPS::Serializer& ser)
{
  ACE_UNUSED_ARG(ser);
  // TODO
  return false;
}

size_t DynamicSample::serialized_size(const DCPS::Encoding& enc) const
{
  ACE_UNUSED_ARG(enc);
  // TODO
  return 0;
}

bool DynamicSample::compare(const DCPS::Sample& other) const
{
  ACE_UNUSED_ARG(other);
  // TODO
  return false;
}

void DynamicTypeSupportImpl::representations_allowed_by_type(DDS::DataRepresentationIdSeq& seq)
{
  seq.length(4);
  seq[0] = DDS::XCDR_DATA_REPRESENTATION;
  seq[1] = DDS::XCDR2_DATA_REPRESENTATION;
  seq[2] = DDS::XML_DATA_REPRESENTATION;
  seq[3] = OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION;
}

DCPS::Extensibility DynamicTypeSupportImpl::base_extensibility() const
{
  DCPS::Extensibility ext = DCPS::FINAL;
  const DDS::ReturnCode_t rc = extensibility(type_, ext);
  if (rc != DDS::RETCODE_OK && DCPS::log_level >= DCPS::LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicTypeSupportImpl::base_extensibility: "
      "could not get correct extensibility for DynamicType %C: %C\n",
      name(), DCPS::retcode_to_string(rc)));
  }
  return ext;
}

DCPS::Extensibility DynamicTypeSupportImpl::max_extensibility() const
{
  DCPS::Extensibility ext = DCPS::FINAL;
  const DDS::ReturnCode_t rc = XTypes::max_extensibility(type_, ext);
  if (rc != DDS::RETCODE_OK && DCPS::log_level >= DCPS::LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicTypeSupportImpl::max_extensibility: "
      "could not get correct max extensibility for DynamicType %C: %C\n",
      name(), DCPS::retcode_to_string(rc)));
  }
  return ext;
}

DDS::DataWriter_ptr DynamicTypeSupportImpl::create_datawriter()
{
  return new DynamicDataWriterImpl();
}

DDS::DataReader_ptr DynamicTypeSupportImpl::create_datareader()
{
  // TODO
  return 0;
}

DDS::DataReader_ptr DynamicTypeSupportImpl::create_multitopic_datareader()
{
  // TODO
  return 0;
}

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
