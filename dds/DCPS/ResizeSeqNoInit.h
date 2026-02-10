/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RESIZE_SEQ_NO_INIT_H
#define OPENDDS_DCPS_RESIZE_SEQ_NO_INIT_H

#include <dds/DCPS/dcps_export.h>
#include <dds/Versioned_Namespace.h>

#include <tao/Basic_Types.h>

#include <ace/config-lite.h>

#ifdef ACE_HAS_CPP11
#  include <type_traits>
#endif

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef ACE_HAS_CPP11

/// For unbounded TAO sequences
template <typename Seq>
auto replace_seq_buffer(Seq& seq, CORBA::ULong new_length, typename Seq::value_type* new_buffer, int) ->
    decltype(seq.replace(CORBA::ULong(), CORBA::ULong(),
      static_cast<typename Seq::value_type*>(nullptr), CORBA::Boolean()), void())
{
  seq.replace(new_length, new_length, new_buffer, true);
}

/// For bounded TAO sequences
template <typename Seq>
void replace_seq_buffer(Seq& seq, CORBA::ULong new_length,
                    typename Seq::value_type* new_buffer, long)
{
  seq.replace(new_length, new_buffer, true);
}

/// Resize TAO sequence without zero-initializing new elements
template <typename Seq>
auto resize_seq_no_init(Seq& seq, CORBA::ULong new_length, int) ->
    decltype(Seq::allocation_traits::allocbuf_noinit(new_length), void())
{
  const CORBA::ULong old_length = seq.length();

  if (new_length <= old_length) {
    seq.length(new_length);
    return;
  }

  typename Seq::value_type* new_buffer = Seq::allocation_traits::allocbuf_noinit(new_length);
  if (old_length > 0) {
    typename Seq::value_type* old_buffer = seq.get_buffer();
    Seq::element_traits::copy_swap_range(old_buffer, old_buffer + old_length, new_buffer);
  }
  replace_seq_buffer(seq, new_length, new_buffer, 0);
}

// std::vector
template <typename Vec>
auto resize_seq_no_init(Vec& vec, typename Vec::size_type new_length, long) ->
    decltype(vec.resize(new_length), void())
{
  vec.resize(new_length);
}

template <typename T>
void resize_seq_no_init(T& seq, CORBA::ULong new_length)
{
  resize_seq_no_init(seq, new_length, 0);
}

#else // C++03 fallbacks

template <typename Seq>
void resize_seq_no_init(Seq& seq, CORBA::ULong new_length)
{
  seq.length(new_length);
}

#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_RESIZE_SEQ_NO_INIT_H
