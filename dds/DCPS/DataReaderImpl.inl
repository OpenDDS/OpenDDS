/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
const OpenDDS::DCPS::DataReaderImpl::StatsMapType&
OpenDDS::DCPS::DataReaderImpl::raw_latency_statistics() const
{
  return this->statistics_;
}

ACE_INLINE
unsigned int&
OpenDDS::DCPS::DataReaderImpl::raw_latency_buffer_size()
{
  return this->raw_latency_buffer_size_;
}

ACE_INLINE
OpenDDS::DCPS::DataCollector<double>::OnFull&
OpenDDS::DCPS::DataReaderImpl::raw_latency_buffer_type()
{
  return this->raw_latency_buffer_type_;
}

ACE_INLINE
void
OpenDDS::DCPS::DataReaderImpl::disable_transport()
{
  this->transport_disabled_ = true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
