/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_HASH_H
#define OPENDDS_DCPS_HASH_H

#include "dds/Versioned_Namespace.h"

#include "dcps_export.h"

#include <cstdint>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

typedef unsigned char MD5Result[16];

OpenDDS_Dcps_Export
void MD5Hash(MD5Result& result, const void* input, size_t size);

OpenDDS_Dcps_Export
uint32_t one_at_a_time_hash(const uint8_t* key, size_t length, uint32_t start = 0u);

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef ACE_HAS_CPP11
#define OOAT_STD_HASH(Key, Export) \
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

#endif // OPENDDS_DCPS_HASH_H
