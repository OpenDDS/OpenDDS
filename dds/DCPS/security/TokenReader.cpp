/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.OpenDDS.org/license.html
*/

#include "dds/DCPS/security/TokenReader.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

TokenReader::TokenReader(const DDS::Security::Token& token_ref)
: token_ref_(token_ref)
, _empty_seq_()
{
}

TokenReader::~TokenReader()
{
}

const char* TokenReader::get_property_value(const std::string& property_name) const
{
  const char* data_ptr = 0;

  CORBA::ULong num_props = token_ref_.properties.length();
  for (CORBA::ULong prop_index = 0; prop_index < num_props; ++prop_index) {
    const DDS::Property_t& prop = token_ref_.properties[prop_index];
    if (0 == property_name.compare(prop.name)) {
      data_ptr = prop.value;
      break;
    }
  }
  return data_ptr;
}

const DDS::OctetSeq& TokenReader::get_bin_property_value(const std::string& property_name) const
{
  CORBA::ULong num_props = token_ref_.binary_properties.length();
  CORBA::ULong prop_index = 0;
  for (prop_index = 0; prop_index < num_props; ++prop_index) {
    if (0 == property_name.compare(token_ref_.binary_properties[prop_index].name)) {
      break;
    }
  }

  if (prop_index >= num_props) {
    return _empty_seq_;
  }

  return token_ref_.binary_properties[prop_index].value;
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
