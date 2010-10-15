// -*- C++ -*-
//
// $Id$

ACE_INLINE
bool&
OpenDDS::Model::Config::verbose()
{
  return this->verbose_;
}

ACE_INLINE
bool
OpenDDS::Model::Config::verbose() const
{
  return this->verbose_;
}

ACE_INLINE
OpenDDS::Model::Config::operator bool() const
{
  return this->configured_;
}

ACE_INLINE
const OpenDDS::Model::Config::ParticipantProfileMap&
OpenDDS::Model::Config::participantProfileMap() const
{
  return this->participantProfileMap_;
}

ACE_INLINE
const OpenDDS::Model::Config::TopicProfileMap&
OpenDDS::Model::Config::topicProfileMap() const
{
  return this->topicProfileMap_;
}

ACE_INLINE
const OpenDDS::Model::Config::PublisherProfileMap&
OpenDDS::Model::Config::publisherProfileMap() const
{
  return this->publisherProfileMap_;
}

ACE_INLINE
const OpenDDS::Model::Config::WriterProfileMap&
OpenDDS::Model::Config::writerProfileMap() const
{
  return this->writerProfileMap_;
}

ACE_INLINE
const OpenDDS::Model::Config::SubscriberProfileMap&
OpenDDS::Model::Config::subscriberProfileMap() const
{
  return this->subscriberProfileMap_;
}

ACE_INLINE
const OpenDDS::Model::Config::ReaderProfileMap&
OpenDDS::Model::Config::readerProfileMap() const
{
  return this->readerProfileMap_;
}

