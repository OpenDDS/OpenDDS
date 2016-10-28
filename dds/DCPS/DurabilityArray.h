/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DURABILITY_ARRAY_H
#define OPENDDS_DURABILITY_ARRAY_H

#include <ace/Array_Base.h>

#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class DurabilityArray
 *
 * @brief Array class that provides a means to reset the
 *        underlying @c ACE_Allocator.
 *
 * This class only exists to provide a means to reset the
 * allocator used by the @c ACE_Array_Base base class.  It has a
 * specific use case, namely to correctly support instances
 * created by a persistent allocator.  The allocator address may
 * change between process runs, meaning the allocator address
 * stored in the persistent @c ACE_Array_Base instance will be
 * invalid.  Use the @c set_allocator() method to reset the
 * allocator address before performing any operations that will
 * require use of the allocator (e.g. increasing the size of the
 * array).
 */
template<typename T>
class DurabilityArray : public ACE_Array_Base<T> {
public:

  DurabilityArray(size_t size,
                  ACE_Allocator * allocator)
    : ACE_Array_Base<T> (size, allocator)
  {}

  DurabilityArray(size_t size,
                  T const & default_value,
                  ACE_Allocator * allocator)
    : ACE_Array_Base<T> (size, default_value, allocator)
  {}

  DurabilityArray(DurabilityArray<T> const & rhs)
    : ACE_Array_Base<T> (rhs.size(), rhs.allocator_)
  {
    for (size_t i = 0; i < this->size_; ++i)
      this->array_[i] = rhs.array_[i];
  }

  ~DurabilityArray()
  {}

  void operator= (DurabilityArray<T> const & rhs)
  {
    DurabilityArray tmp(rhs);
    this->swap(rhs);
  }

  /// Reset allocator
  void set_allocator(ACE_Allocator * allocator)
  {
    if (allocator == 0)
      allocator = ACE_Allocator::instance();

    this->allocator_ = allocator;
  }

  void swap(DurabilityArray<T> & rhs)
  {
    std::swap(this->max_size_, rhs.max_size_);
    std::swap(this->cur_size_, rhs.current_size_);
    std::swap(this->array_, rhs.array_);
    std::swap(this->allocator_, rhs.allocator_);
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DURABILITY_ARRAY_H */
