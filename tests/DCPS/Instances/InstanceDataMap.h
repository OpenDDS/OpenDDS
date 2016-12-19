// -*- C++ -*-
//
#ifndef INSTANCE_DATA_MAP_H
#define INSTANCE_DATA_MAP_H

#include  "dds/DdsDcpsInfoUtilsC.h"
#include  "ace/Thread_Mutex.h"

#include <set>
#include <map>

template<typename Type>
struct TypeSequenceLessThan
{
  bool operator() (
    const Type& v1,
    const Type& v2) const
  {
    return v1.sample_sequence < v2.sample_sequence;
  }
};

template<typename MessageType>
class InstanceDataMap
{
  public:
    typedef MessageType message_type;
    typedef typename std::set< message_type, TypeSequenceLessThan<message_type> > DataSet;
    typedef typename std::map < ::DDS::InstanceHandle_t, DataSet > DataMap;

    InstanceDataMap() : current_sequence_number_ (0) {}
    virtual ~InstanceDataMap(){}

    int insert (::DDS::InstanceHandle_t handle, message_type& sample)
    {
      ACE_GUARD_RETURN (ACE_Thread_Mutex,
                        guard,
                        this->lock_,
                        -1);

      current_sequence_number_ ++;
      sample.sample_sequence = current_sequence_number_;

      DataSet& dataset = map_[handle];
      std::pair< typename DataSet::iterator,bool> map_pair = dataset.insert( message_type(sample) );

      return map_pair.second == true ? 0 : -1;
    }

    int remove (::DDS::InstanceHandle_t handle, message_type& sample)
    {
      ACE_GUARD_RETURN (ACE_Thread_Mutex,
                        guard,
                        this->lock_,
                        -1);

      typename DataMap::iterator it = map_.find (handle);
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

    ssize_t num_instances()
    {
      ACE_GUARD_RETURN (ACE_Thread_Mutex,
                        guard,
                        this->lock_,
                        -1);

      return this->map_.size();
    }

    ssize_t num_samples()
    {
      ACE_GUARD_RETURN (ACE_Thread_Mutex,
                        guard,
                        this->lock_,
                        -1);
      ssize_t num_samples = 0;
      typename DataMap::iterator it;
      for (it = map_.begin (); it != map_.end (); it ++)
      {
        num_samples += it->second.size ();
      }
      return num_samples;
    }

    bool is_empty ()
    {
      return map_.empty ();
    }

  private:

    DataMap          map_;
    ACE_Thread_Mutex lock_;
    long             current_sequence_number_;
};

#endif /* INSTANCE_DATA_MAP_H */
