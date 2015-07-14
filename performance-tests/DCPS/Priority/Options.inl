// -*- C++ -*-
//

ACE_INLINE
bool&
Test::Options::verbose()
{
  return this->verbose_;
}

ACE_INLINE
bool
Test::Options::verbose() const
{
  return this->verbose_;
}

ACE_INLINE
unsigned long&
Test::Options::domain()
{
  return this->domain_;
}

ACE_INLINE
unsigned long
Test::Options::domain() const
{
  return this->domain_;
}

ACE_INLINE
long&
Test::Options::duration()
{
  return this->duration_;
}

ACE_INLINE
long
Test::Options::duration() const
{
  return this->duration_;
}

ACE_INLINE
Test::Options::TransportType
Test::Options::transportType() const
{
  return this->transportType_;
}

ACE_INLINE
Test::Options::TransportType&
Test::Options::transportType()
{
  return this->transportType_;
}

ACE_INLINE
OPENDDS_STRING
Test::Options::transportKey() const
{
  return this->transportKey_;
}

ACE_INLINE
OPENDDS_STRING&
Test::Options::transportKey()
{
  return this->transportKey_;
}

ACE_INLINE
std::string
Test::Options::topicName() const
{
  return this->topicName_;
}

ACE_INLINE
std::string&
Test::Options::topicName()
{
  return this->topicName_;
}

ACE_INLINE
std::string&
Test::Options::rawOutputFilename()
{
  return this->rawOutputFilename_;
}

ACE_INLINE
std::string
Test::Options::rawOutputFilename() const
{
  return this->rawOutputFilename_;
}

ACE_INLINE
unsigned int&
Test::Options::raw_buffer_size()
{
  return this->raw_buffer_size_;
}

ACE_INLINE
unsigned int
Test::Options::raw_buffer_size() const
{
  return this->raw_buffer_size_;
}

ACE_INLINE
OpenDDS::DCPS::DataCollector< double>::OnFull&
Test::Options::raw_buffer_type()
{
  return this->raw_buffer_type_;
}

ACE_INLINE
OpenDDS::DCPS::DataCollector< double>::OnFull
Test::Options::raw_buffer_type() const
{
  return this->raw_buffer_type_;
}

ACE_INLINE
const Test::Options::ProfileContainer&
Test::Options::profiles() const
{
  return this->publicationProfiles_;
}

