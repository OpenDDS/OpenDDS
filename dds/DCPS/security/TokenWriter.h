/*
*
*
* Distributed under the DDS License.
* See: http://www.DDS.org/license.html
*/



#ifndef DDS_DCPS_TOKEN_WRITER_IMPL_H
#define DDS_DCPS_TOKEN_WRITER_IMPL_H

#include "dds/DCPS/security/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"

#include "TokenReader.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {


/**
* @class TokenWriter
*
* @brief Implements some simple wrapper functions to provide a non-const API
* around the Token data structure as specified in the DDS security specification
*
* See the DDS security specification, OMG formal/17-09-20, for a description of
* the interface this class is implementing.
*
*/

class  TokenWriter
{
public:
  TokenWriter(DDS::Security::Token& token_ref);
  TokenWriter(DDS::Security::Token& token_ref,
  const std::string& class_id,
  unsigned int num_properties, 
  unsigned int num_bin_properties);

  virtual ~TokenWriter();

  const TokenReader& get_reader();

  void set_class_id(const std::string& class_name);
  void set_property_count(unsigned int num_properties);
  void set_bin_property_count(unsigned int num_properties);

  void set_property(int prop_index, const char* prop_name, const char* prop_value, bool propagate);
  void set_bin_property(int prop_index, const char* prop_name, const DDS::OctetSeq& prop_value, bool propagate);
  void set_bin_property(int prop_index, const char* prop_name, const std::string& prop_value, bool propagate);

private:
  DDS::Security::Token& token_ref_;
  OpenDDS::Security::TokenReader reader_;
};

inline const TokenReader& TokenWriter::get_reader()
{
  return reader_;
}

inline void TokenWriter::set_class_id(const std::string& class_id)
{
  token_ref_.class_id = class_id.c_str();
}
inline void TokenWriter::set_property_count(unsigned int num_properties)
{
  token_ref_.properties.length(num_properties);
}
inline void TokenWriter::set_bin_property_count(unsigned int num_properties)
{
  token_ref_.properties.length(num_properties);
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

#endif
