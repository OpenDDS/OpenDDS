/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPS_BASEMESSAGEUTILS_H
#define RTPS_BASEMESSAGEUTILS_H

#include "dds/DCPS/Serializer.h"
#include "RtpsBaseMessageTypesC.h"
#include "md5.h"

#include <cstring>

namespace OpenDDS {
namespace RTPS {

template <typename T>
void marshal_key_hash(const T& msg, KeyHash_t& hash) {
  using OpenDDS::DCPS::Serializer;

  OpenDDS::DCPS::KeyOnly<const T> ko(msg);

  static const size_t HASH_LIMIT = 16;
  std::memset(hash.value, 0, HASH_LIMIT);

  // Key Hash must use big endian ordering.
  // Native==Little endian means we need to swap
#if defined ACE_LITTLE_ENDIAN
  static const bool swap_bytes = true;
#else
  static const bool swap_bytes = false;
#endif

  if (gen_is_bounded_size(ko) &&
      gen_max_marshaled_size(ko, true /*align*/) <= HASH_LIMIT) {
    // If it is bounded and can always fit in 16 bytes, we will use the
    // marshaled key
    ACE_Message_Block mb(HASH_LIMIT);
    Serializer out_serializer(&mb, swap_bytes, Serializer::ALIGN_INITIALIZE);
    out_serializer << ko;
    std::memcpy(hash.value, mb.rd_ptr(), mb.length());

  } else {
    // We will use the hash of the marshaled key
    size_t size = 0, padding = 0;
    gen_find_size(ko, size, padding);
    ACE_Message_Block mb(size + padding);
    Serializer out_serializer(&mb, swap_bytes, Serializer::ALIGN_INITIALIZE);
    out_serializer << ko;

    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, mb.rd_ptr(), static_cast<unsigned long>(mb.length()));
    MD5_Final(hash.value, &ctx);
  }
}

void assign(OpenDDS::RTPS::GuidPrefix_t& dest,
            const OpenDDS::RTPS::GuidPrefix_t& src)
{
  std::memcpy(&dest[0], &src[0], sizeof(GuidPrefix_t));
}

void assign(OpenDDS::RTPS::OctetArray16& dest,
            const OpenDDS::RTPS::OctetArray16& src)
{
  std::memcpy(&dest[0], &src[0], sizeof(OpenDDS::RTPS::OctetArray16));
}

void assign(OpenDDS::RTPS::OctetArray16& dest,
            const ACE_CDR::ULong& ipv4addr_be)
{
  std::memset(&dest[0], 0, 12);
  dest[12] = ipv4addr_be >> 24;
  dest[13] = ipv4addr_be >> 16;
  dest[14] = ipv4addr_be >> 8;
  dest[15] = ipv4addr_be;
}


}
}

#endif /* RTPS_BASEMESSAGETYPES_H */
