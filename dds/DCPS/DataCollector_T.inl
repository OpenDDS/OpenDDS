/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifndef OPENDDS_SAFETY_PROFILE
template<typename DatumType>
ACE_INLINE
std::ostream& operator<<(
  std::ostream& str,
  const DataCollector<DatumType>& value)
{
  return value.insert(str);
}
#endif //OPENDDS_SAFETY_PROFILE

template<typename DatumType>
ACE_INLINE
DataCollector<DatumType>::DataCollector(unsigned int bound, OnFull onFull)
  : buffer_(bound),
    writeAt_(0),
    bound_(bound),
    full_(false),
    onFull_(onFull)
{
  if (this->onFull_ == Unbounded) {
    this->buffer_.clear();
  }
}

#ifndef OPENDDS_SAFETY_PROFILE
template<typename DatumType>
ACE_INLINE
DataCollector<DatumType>&
DataCollector<DatumType>::operator<<(DatumType datum)
{
  this->collect(datum);
  return *this;
}
#endif //OPENDDS_SAFETY_PROFILE

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
