/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_HASH_H
#define OPENDDS_DCPS_HASH_H

#include "dds/Versioned_Namespace.h"

#include "dcps_export.h"

#ifdef ACE_HAS_CPP11
#include <cstdint>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

typedef unsigned char MD5Result[16];

OpenDDS_Dcps_Export
void MD5Hash(MD5Result& result, const void* input, size_t size);

#ifdef ACE_HAS_CPP11
OpenDDS_Dcps_Export
inline uint32_t one_at_a_time_hash(const uint8_t* key, size_t length, uint32_t start_hash = 0u)
{
  uint32_t hash = start_hash;
  size_t i = 0;
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}
#endif

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef ACE_HAS_CPP11
#define OPENDDS_OOAT_STD_HASH(Key, Export) \
  namespace std \
  { \
  template<> struct Export hash<Key> \
  { \
    std::size_t operator()(const Key& val) const noexcept \
    { \
      return static_cast<size_t>(OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&val), sizeof (Key))); \
    } \
  }; \
  }
#endif

#ifdef ACE_HAS_CPP11
#define OPENDDS_OOAT_CUSTOM_HASH(Key, Export, Name) \
  struct Export Name \
  { \
    std::size_t operator()(const Key& val) const noexcept \
    { \
      return static_cast<size_t>(OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&val), sizeof (Key))); \
    } \
  };
#endif

#endif // OPENDDS_DCPS_HASH_H
