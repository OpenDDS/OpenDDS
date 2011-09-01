/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BaseMessageTypes.h"
#include "MessageTypes.h"
#include "BaseMessageUtils.h"

namespace {
  using OpenDDS::DCPS::KeyOnly;
  using OpenDDS::DCPS::Serializer;

  struct TestMsg {
    ACE_CDR::ULong key;
    TAO::String_Manager value;
  };

  bool gen_is_bounded_size(const TestMsg&) { return false; }

  size_t gen_max_marshaled_size(const TestMsg&, bool /*align*/) { return 0; }

  void gen_find_size(const TestMsg& stru, size_t& size, size_t& padding)
  {
    if ((size + padding) % 4) {
      padding += 4 - ((size + padding) % 4);
    }
    size += 9 + ACE_OS::strlen(stru.value);
  }

  bool gen_is_bounded_size(KeyOnly<const TestMsg>) { return true; }

  size_t gen_max_marshaled_size(KeyOnly<const TestMsg>, bool /*align*/) { return 4; }

  void gen_find_size(KeyOnly<const TestMsg>, size_t& size, size_t& padding)
  {
    if ((size + padding) % 4) {
      padding += 4 - ((size + padding) % 4);
    }
    size += 4;
  }

  bool operator<<(Serializer& strm, KeyOnly<const TestMsg> stru)
  {
    return strm << stru.t.key;
  }

  void instantiate_templates()
  {
    TestMsg tm;
    OpenDDS::RTPS::KeyHash_t hash;
    marshal_key_hash(tm, hash);
  }
}
