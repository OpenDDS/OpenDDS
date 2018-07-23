/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.OpenDDS.org/license.html
*/

#include "dds/DCPS/security/TokenReader.h"
#include "dds/DCPS/sequence_iterator.h"
#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
  using namespace DCPS;

  typedef SequenceIterator<DDS::PropertySeq> iter_t;
  typedef has_property<DDS::Property_t> has_property;

  DDS::PropertySeq& props = const_cast<DDS::PropertySeq&>(token_ref_.properties);

  iter_t begin = sequence_begin(props), end = sequence_end(props);
  iter_t result = std::find_if(begin, end, has_property(property_name));

  if (result != end) {
    return result->value;
  }

  return 0;
}

const DDS::OctetSeq& TokenReader::get_bin_property_value(const std::string& property_name) const
{
  using namespace DCPS;

  typedef SequenceIterator<DDS::BinaryPropertySeq> iter_t;
  typedef has_property<DDS::BinaryProperty_t> has_property;

  DDS::BinaryPropertySeq& props = const_cast<DDS::BinaryPropertySeq&>(token_ref_.binary_properties);

  iter_t begin = sequence_begin(props), end = sequence_end(props);
  iter_t result = std::find_if(begin, end, has_property(property_name));

  if (result != end) {
    return result->value;
  }

  return _empty_seq_;
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
