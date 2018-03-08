/*
*
*
* Distributed under the DDS License.
* See: http://www.DDS.org/license.html
*/
#include "dds/DCPS/security/TokenWriter.h"
#include <cstring>
#
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

TokenWriter::TokenWriter(DDS::Security::Token& token_ref)
: token_ref_(token_ref)
, reader_(token_ref)
{
}

TokenWriter::TokenWriter(
  DDS::Security::Token& token_ref,
  const std::string& class_id,
  unsigned int num_properties,
  unsigned int num_bin_properties)
: token_ref_(token_ref)
, reader_(token_ref)
{
  token_ref_.class_id = class_id.c_str();
  token_ref_.properties.length(num_properties);
  token_ref_.binary_properties.length(num_bin_properties);
}

TokenWriter::~TokenWriter()
{
}

void TokenWriter::set_property(int prop_index, const char* prop_name, const char* prop_value, bool propagate)
{
  DDS::Property_t& prop_ref = token_ref_.properties[prop_index];
  prop_ref.name = prop_name;
  prop_ref.value = prop_value;
  prop_ref.propagate = propagate;
}

void TokenWriter::set_bin_property(int prop_index, const char* prop_name, const DDS::OctetSeq& prop_value, bool propagate)
{
  DDS::BinaryProperty_t& prop_ref = token_ref_.binary_properties[prop_index];
  prop_ref.name = prop_name;
  prop_ref.value = prop_value;
  prop_ref.propagate = propagate;
}

void TokenWriter::set_bin_property(int prop_index, const char* prop_name, const std::string& prop_value, bool propagate)
{
  DDS::BinaryProperty_t& prop_ref = token_ref_.binary_properties[prop_index];
  prop_ref.name = prop_name;
  prop_ref.propagate = propagate;
  prop_ref.value.length(prop_value.length());
  std::memcpy(prop_ref.value.get_buffer(),
              prop_value.c_str(),
              prop_ref.value.length());
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
