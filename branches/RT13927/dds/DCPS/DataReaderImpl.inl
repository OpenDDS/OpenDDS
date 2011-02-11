/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

ACE_INLINE
DDS::DataReader_ptr
OpenDDS::DCPS::DataReaderImpl::get_dr_obj_ref()
{
  return DDS::DataReader::_duplicate(dr_local_objref_.in()) ;
}

ACE_INLINE
int
OpenDDS::DCPS::WriterInfo::received_activity(const ACE_Time_Value& when)
{
  last_liveliness_activity_time_ = when;

  if (state_ != ALIVE) { // NOT_SET || DEAD
    reader_->writer_became_alive(*this, when);
    return 0;
  }

  //TBD - is the "was alive" return value used?
  return 1;
}

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
