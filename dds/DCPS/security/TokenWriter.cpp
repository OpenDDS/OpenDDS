/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.OpenDDS.org/license.html
*/
#include "TokenWriter.h"
#include <cstring>
#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

TokenWriter::TokenWriter(DDS::Security::Token& token_ref)
: binary_property_inserter_(token_ref.binary_properties)
, property_inserter_(token_ref.properties)
, token_ref_(token_ref)
, reader_(token_ref)
{

}

TokenWriter::TokenWriter(DDS::Security::Token& token_ref, const std::string& class_id)
: binary_property_inserter_(token_ref.binary_properties)
, property_inserter_(token_ref.properties)
, token_ref_(token_ref)
, reader_(token_ref)
{
  token_ref.class_id = class_id.c_str();
}

TokenWriter::~TokenWriter()
{
}

void TokenWriter::add_property(const char* prop_name, const char* prop_value, bool propagate)
{
  DDS::Property_t p;
  p.name = prop_name;
  p.value = prop_value;
  p.propagate = propagate;
  *property_inserter_ = p;
}

void TokenWriter::add_property(const char* prop_name, const DDS::OctetSeq& prop_value, bool propagate)
{
  std::ostringstream out;
  out.write(reinterpret_cast<const char*>(prop_value.get_buffer()), prop_value.length());

  DDS::Property_t p;
  p.name = prop_name;
  p.value = out.str().c_str();
  p.propagate = propagate;
  *property_inserter_ = p;
}

void TokenWriter::add_bin_property(const char* prop_name, const DDS::OctetSeq& prop_value, bool propagate)
{
  DDS::BinaryProperty_t p;
  p.name = prop_name;
  p.value = prop_value;
  p.propagate = propagate;
  *binary_property_inserter_ = p;
}

void TokenWriter::add_bin_property(const char* prop_name, const std::string& prop_value, bool propagate)
{
  DDS::BinaryProperty_t p;
  p.name = prop_name;
  p.propagate = propagate;
  p.value.length(static_cast<unsigned int>(prop_value.size() + 1 /* For null */));
  std::memcpy(p.value.get_buffer(),
              prop_value.c_str(),
              p.value.length());
  *binary_property_inserter_ = p;
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
