/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */



#ifndef OPENDDS_DCPS_SECURITY_TOKENREADER_H
#define OPENDDS_DCPS_SECURITY_TOKENREADER_H

#include "OpenDDS_Security_Export.h"

#include <dds/DdsSecurityCoreC.h>
#include <dds/Versioned_Namespace.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

/**
* @class TokenReader
*
* @brief Implements some simple wrapper functions to provide a const API
* around the Token data structure as specified in the DDS security specification
*
* See the DDS security specification, OMG formal/17-09-20, for a description of
* the interface this class is implementing.
*
*/

class OpenDDS_Security_Export TokenReader
{
public:
  explicit TokenReader(const DDS::Security::Token& token_ref);
  virtual ~TokenReader();

  bool is_nil() const;
  const char* get_property_value(const std::string& property_name) const;
  const DDS::OctetSeq& get_bin_property_value(const std::string& property_name) const;

  CORBA::ULong get_num_properties() const;
  CORBA::ULong get_num_bin_properties() const;

private:
  const DDS::Security::Token& token_ref_;
  const DDS::OctetSeq _empty_seq_;
};

inline bool TokenReader::is_nil() const
{
  return ((token_ref_.binary_properties.length() == 0)
    && (token_ref_.properties.length() == 0)
    && (token_ref_.class_id[0] == '\0'));
}

inline CORBA::ULong TokenReader::get_num_properties() const
{
  return token_ref_.properties.length();
}

inline CORBA::ULong TokenReader::get_num_bin_properties() const
{
  return token_ref_.binary_properties.length();
}
} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
