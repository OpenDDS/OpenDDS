// -*- C++ -*-
//
#ifndef INSTANCE_DATA_MAP_H
#define INSTANCE_DATA_MAP_H

#include  "dds/DdsDcpsInfrastructureC.h"
#include  "tests/DCPS/FooType3Unbounded/FooDefC.h"
#include  "ace/Synch.h"

#include <set>
#include <map>

class InstanceDataMap
{
  public:
    struct FooSequenceLessThan
    {
      bool operator() (
        const ::Xyz::Foo& v1,
        const ::Xyz::Foo& v2) const
      {
        return v1.sample_sequence < v2.sample_sequence;
      }
    };

    typedef std::set < ::Xyz::Foo, FooSequenceLessThan > DataSet;
    typedef std::map < ::DDS::InstanceHandle_t, DataSet > DataMap;


    InstanceDataMap();
    virtual ~InstanceDataMap();

    int insert (::DDS::InstanceHandle_t, ::Xyz::Foo& sample);
    int remove (::DDS::InstanceHandle_t, ::Xyz::Foo& sample);

    ssize_t num_instances();
    ssize_t num_samples();
    bool    is_empty ();

  private:

    DataMap          map_;
    ACE_Thread_Mutex lock_;
    long             current_sequence_number_;
};

#endif /* INSTANCE_DATA_MAP_H */
