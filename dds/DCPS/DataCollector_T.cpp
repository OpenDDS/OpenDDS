/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataCollector_T.h"
#include "dds/DCPS/SafetyProfileStreams.h"

#if !defined (__ACE_INLINE__)
#include "DataCollector_T.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename DatumType>
DataCollector<DatumType>::~DataCollector()
{
}

template<typename DatumType>
void
DataCollector<DatumType>::collect(const DatumType& datum)
{
  if (this->onFull_ == Unbounded) {
    this->buffer_.push_back(datum);

  } else {
    // writeAt == bound only when we either have no buffer (bound == 0) or
    // when we are full with no wrapping (writeAt == bound)
    if (this->writeAt_ != this->bound_) {
      this->buffer_[ this->writeAt_++] = datum;

      // This datum filled the buffer.
      if (this->writeAt_ == this->bound_) {
        this->full_  = true;

        if (this->onFull_ == KeepNewest) {
          this->writeAt_ = 0;
        }
      }
    }
  }
}

template<typename DatumType>
unsigned int
DataCollector<DatumType>::size() const
{
  if (this->onFull_ == Unbounded) return static_cast<unsigned int>(this->buffer_.size());

  else if (this->full_)           return this->bound_;

  else                            return this->writeAt_;
}

#ifndef OPENDDS_SAFETY_PROFILE
template<typename DatumType>
std::ostream&
DataCollector<DatumType>::insert(std::ostream& str) const
{
  std::ofstream initStrState;
  initStrState.copyfmt(str);

  str.precision(5);
  str << std::scientific;

  // Oldest data first.
  if (this->full_) {
    for (unsigned int index = this->writeAt_; index < this->bound_; ++index) {
      str << this->buffer_[ index] << std::endl;
    }
  }

  // Bounded case.
  int end = this->writeAt_;

  if (end == 0) {
    // Unbounded case.
    end = static_cast<int>(this->buffer_.size());
  }

  // Newest data last.
  for (int index = 0; index < end; ++index) {
    str << this->buffer_[ index] << std::endl;
  }

  str.copyfmt(initStrState);
  return str;
}
#endif //OPENDDS_SAFETY_PROFILE

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
