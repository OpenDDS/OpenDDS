/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RESIZE_SEQ_NO_INIT_H
#define OPENDDS_DCPS_RESIZE_SEQ_NO_INIT_H

#include "dcps_export.h"

#include <dds/Versioned_Namespace.h>

#include <tao/Basic_Types.h>

#include <ace/config-lite.h>

#ifdef ACE_HAS_CPP11
#  include <vector>
#  include <memory>
#endif

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace FaceTypes {
  template <typename T, typename Bounds, typename Elts>
  class Sequence;
}

namespace DCPS {

bool OpenDDS_Dcps_Export get_init_in_optional_init_allocator();

/// Resize unbounded FACE sequences without zero-initializing new elements.
template <typename T, typename Bounds, typename Elts>
void resize_unbounded_seq_no_init(
  OpenDDS::FaceTypes::Sequence<T, Bounds, Elts>& seq,
  typename OpenDDS::FaceTypes::Sequence<T, Bounds, Elts>::size_type new_length)
{
  // It looks like FACE sequences already don't zero-initialize their memory
  // for primitives.
  seq.length(new_length);
}

/// Resize bounded FACE sequences without zero-initializing new elements.
template <typename T, typename Bounds, typename Elts>
void resize_bounded_seq_no_init(
  OpenDDS::FaceTypes::Sequence<T, Bounds, Elts>& seq,
  typename OpenDDS::FaceTypes::Sequence<T, Bounds, Elts>::size_type new_length)
{
  resize_unbounded_seq_no_init(seq, new_length);
}

#ifdef ACE_HAS_CPP11
struct OpenDDS_Dcps_Export InitInOptionalInitAllocator {
  const bool prev_value;

  InitInOptionalInitAllocator(bool value);
  ~InitInOptionalInitAllocator();
};

template <typename T, typename Alloc = std::allocator<T>>
class OptionalInitAllocator : public Alloc {
protected:
  using Traits = std::allocator_traits<Alloc>;

public:
  template<typename U>
  struct rebind {
      using other = OptionalInitAllocator<U, typename Traits::template rebind_alloc<U>>;
  };

  using Alloc::Alloc;

  // Default-construction
  template <typename U>
  void construct(U* ptr) noexcept(std::is_nothrow_default_constructible<U>::value)
  {
    if (get_init_in_optional_init_allocator()) {
      ::new (static_cast<void*>(ptr)) U();
    } else {
      ::new (static_cast<void*>(ptr)) U;
    }
  }

  // Construction with arguments
  template <typename U, typename... Args>
  void construct(U* ptr, Args&&... args)
  {
    Traits::construct(static_cast<Alloc&>(*this), ptr, std::forward<Args>(args)...);
  }
};

template <typename T>
using OptionalInitVector = std::vector<T, OptionalInitAllocator<T>>;

/// Try to resize unbounded std::vector without zero-initializing new elements.
template <typename T, typename Alloc>
void resize_unbounded_seq_no_init(std::vector<T, Alloc>& vec, typename std::vector<T, Alloc>::size_type new_length)
{
  // This is the most painful version of resize_seq_no_init. The std::vector
  // needs a custom allocator (OptionalInitVector). This will almost certainly
  // be slower than a vanilla std::vector unless optimization is enabled.
  InitInOptionalInitAllocator skip_init(false);
  vec.resize(new_length);
}

/// Try to resize unbounded std::vector without zero-initializing new elements.
template <typename T, typename Alloc>
void resize_bounded_seq_no_init(std::vector<T, Alloc>& vec, typename std::vector<T, Alloc>::size_type new_length)
{
  resize_unbounded_seq_no_init(vec, new_length);
}
#endif

template <typename Seq>
typename Seq::value_type* resize_tao_seq_no_init(Seq& seq, CORBA::ULong new_length)
{
  const size_t old_length = seq.length();

  // If not growing, just call length.
  if (new_length <= old_length) {
    seq.length(new_length);
    return 0;
  }

  // If we're growing the sequence, then create a bigger copy of the buffer
  // with the new elements left uninitialized. Then that's passed it to the
  // right version of replace.
  typename Seq::value_type* new_buffer = Seq::allocation_traits::allocbuf_noinit(new_length);
  if (old_length > 0) {
    typename Seq::value_type* old_buffer = seq.get_buffer();
    Seq::element_traits::copy_swap_range(old_buffer, old_buffer + old_length, new_buffer);
  }
  return new_buffer;
}

/// C++03 SFINAE helper to prevent std::vector and FACE sequences from matching
/// since there is no single TAO sequence type.
/// TODO: replace with std::void_t in C++17
template <typename T>
struct VoidIfExists {
  typedef void type;
};

/// Resize unbounded TAO sequences without zero-initializing new elements.
template <typename Seq>
typename VoidIfExists<typename Seq::allocation_traits>::type
  resize_unbounded_seq_no_init(Seq& seq, CORBA::ULong new_length)
{
  typename Seq::value_type* new_buffer = resize_tao_seq_no_init(seq, new_length);
  if (new_buffer) {
    seq.replace(new_length, new_length, new_buffer, true);
  }
}

/// Resize bounded TAO sequences without zero-initializing new elements.
template <typename Seq>
typename VoidIfExists<typename Seq::allocation_traits>::type
  resize_bounded_seq_no_init(Seq& seq, CORBA::ULong new_length)
{
  typename Seq::value_type* new_buffer = resize_tao_seq_no_init(seq, new_length);
  if (new_buffer) {
    seq.replace(new_length, new_buffer, true);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_RESIZE_SEQ_NO_INIT_H
