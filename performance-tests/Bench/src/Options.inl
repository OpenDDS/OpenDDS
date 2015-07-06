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
Test::Options::rawBufferSize()
{
  return this->rawBufferSize_;
}

ACE_INLINE
unsigned int
Test::Options::rawBufferSize() const
{
  return this->rawBufferSize_;
}

ACE_INLINE
OpenDDS::DCPS::DataCollector< double>::OnFull&
Test::Options::rawBufferType()
{
  return this->rawBufferType_;
}

ACE_INLINE
OpenDDS::DCPS::DataCollector< double>::OnFull
Test::Options::rawBufferType() const
{
  return this->rawBufferType_;
}

ACE_INLINE
const Test::Options::ParticipantProfileMap&
Test::Options::participantProfileMap() const
{
  return this->participantProfileMap_;
}

ACE_INLINE
const Test::Options::TopicProfileMap&
Test::Options::topicProfileMap() const
{
  return this->topicProfileMap_;
}

ACE_INLINE
const Test::Options::PublicationProfileMap&
Test::Options::publicationProfileMap() const
{
  return this->publicationProfileMap_;
}

ACE_INLINE
const Test::Options::SubscriptionProfileMap&
Test::Options::subscriptionProfileMap() const
{
  return this->subscriptionProfileMap_;
}

