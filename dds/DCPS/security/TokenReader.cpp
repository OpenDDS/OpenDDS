/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.OpenDDS.org/license.html
*/

#include "TokenReader.h"
#include "dds/DCPS/SequenceIterator.h"
#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
using namespace OpenDDS::DCPS;

namespace OpenDDS {
namespace Security {

template<typename PropType>
struct has_property
{
  has_property(const std::string& name) : name_(name) {}

  bool operator() (const PropType& property)
  {
    return (0 == name_.compare(property.name));
  }

private:
  const std::string& name_;
};

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
  typedef ConstSequenceIterator<const DDS::PropertySeq> iter_t;
  typedef has_property<const DDS::Property_t> has_property;

  iter_t begin = const_sequence_begin(token_ref_.properties),
         end = const_sequence_end(token_ref_.properties);

  iter_t result = std::find_if(begin, end, has_property(property_name));

  if (result != end) {
    return result->value;
  }

  return 0;
}

const DDS::OctetSeq& TokenReader::get_bin_property_value(const std::string& property_name) const
{
  typedef ConstSequenceIterator<const DDS::BinaryPropertySeq> iter_t;
  typedef has_property<const DDS::BinaryProperty_t> has_property;

  iter_t begin = const_sequence_begin(token_ref_.binary_properties),
         end = const_sequence_end(token_ref_.binary_properties);

  iter_t result = std::find_if(begin, end, has_property(property_name));

  if (result != end) {
    return result->value;
  }

  return _empty_seq_;
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
