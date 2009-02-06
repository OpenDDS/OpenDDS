// -*- C++ -*-
//
// $Id$

ACE_INLINE
::DDS::DataReader_ptr
OpenDDS::DCPS::DataReaderImpl::get_dr_obj_ref() {
#if 1
  //TBD this may be faster than servant_to_reference - ?significantly?
  return ::DDS::DataReader::_duplicate (dr_local_objref_.in ()) ;
#else
#if 0
  // we don't really need the RemoteDataReader OR a DataReder OR will do.
  return this->get_datareaderremote_obj_ref ();
#else
  ::DDS::DataReader_ptr reader_obj
      = ::OpenDDS::DCPS::servant_to_reference< ::DDS::DataReader,
                                           OpenDDS::DCPS::DataReaderImpl,
                                           ::DDS::DataReader_ptr>
            (this);
  // OpenDDS::DCPS::servant_to_reference narrowed so it has been dup'd
  return reader_obj;
#endif
#endif
}


ACE_INLINE
int
OpenDDS::DCPS::WriterInfo::received_activity (const ACE_Time_Value& when)
{
  last_liveliness_activity_time_ = when;
  if (state_ != ALIVE) // NOT_SET || DEAD
    {
      reader_->writer_became_alive (writer_id_, when, state_);
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
OpenDDS::DCPS::DataCollector< double>::OnFull&
OpenDDS::DCPS::DataReaderImpl::raw_latency_buffer_type()
{
  return this->raw_latency_buffer_type_;
}
 


