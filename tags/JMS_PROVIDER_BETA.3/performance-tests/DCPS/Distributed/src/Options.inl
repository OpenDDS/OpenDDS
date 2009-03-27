// -*- C++ -*-
//
// $Id$

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
bool&
Test::Options::configured()
{
  return this->configured_;
}

ACE_INLINE
bool
Test::Options::configured() const
{
  return this->configured_;
}

ACE_INLINE
Test::Options::operator bool() const
{
  return this->configured_;
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
const Test::Options::ParticipantMap&
Test::Options::participantMap() const
{
  return this->participantMap_;
}

ACE_INLINE
const Test::Options::TopicMap&
Test::Options::topicMap() const
{
  return this->topicMap_;
}

ACE_INLINE
const Test::Options::PublicationMap&
Test::Options::publicationMap() const
{
  return this->publicationMap_;
}

ACE_INLINE
const Test::Options::SubscriptionMap&
Test::Options::subscriptionMap() const
{
  return this->subscriptionMap_;
}

