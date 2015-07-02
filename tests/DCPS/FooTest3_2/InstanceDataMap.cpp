// -*- C++ -*-
//
#include "InstanceDataMap.h"


InstanceDataMap::InstanceDataMap()
: current_sequence_number_ (0)
{
}


InstanceDataMap::~InstanceDataMap()
{
}

int
InstanceDataMap::insert (::DDS::InstanceHandle_t handle, ::Xyz::Foo& sample)
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->lock_,
                    -1);

  current_sequence_number_ ++;
  sample.sample_sequence = current_sequence_number_;
  std::pair<DataSet::iterator, bool> pair
        = map_[handle].insert(DataSet::value_type(sample));

  return pair.second == true ? 0 : -1;
}

int
InstanceDataMap::remove (::DDS::InstanceHandle_t handle, ::Xyz::Foo& sample)
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->lock_,
                    -1);

  DataMap::iterator it = map_.find (handle);
  if (it == map_.end ())
  {
    return -1;
  }

  size_t size = it->second.erase (sample);
  if (size != 1)
  {
    return -1;
  }

  if (it->second.size () == 0)
  {
    map_.erase (handle);
  }
  return 0;
}

ssize_t
InstanceDataMap::num_instances()
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->lock_,
                    -1);

  return this->map_.size();
}

ssize_t
InstanceDataMap::num_samples()
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->lock_,
                    -1);
  ssize_t num_samples = 0;
  DataMap::iterator it;
  for (it = map_.begin (); it != map_.end (); it ++)
  {
    num_samples += it->second.size ();
  }
  return num_samples;
}

bool
InstanceDataMap::is_empty ()
{
  return map_.empty ();
}
