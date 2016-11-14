// -*- C++ -*-
//
#ifndef INSTANCE_DATA_MAP_H
#define INSTANCE_DATA_MAP_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "tests/DCPS/FooType5/FooDefC.h"

#include "ace/Thread_Mutex.h"
#include "ace/Containers_T.h"
#include "ace/Hash_Map_Manager.h"

class InstanceDataMap
{
  public:

    typedef ACE_Array <int> DataArray;
    typedef ACE_Hash_Map_Manager < int, DataArray, ACE_Null_Mutex > DataMap;

    InstanceDataMap();

    virtual ~InstanceDataMap();

    void init();
    int add (Xyz::Foo& sample);
    int add (Xyz::FooNoKey& sample);

    bool test_passed(int expected);

  private:

    DataMap*          maps_;
    ACE_Thread_Mutex  lock_;
    int               num_messages_expected_;
    int               num_receives_per_sample_;
};


#endif /* INSTANCE_DATA_MAP_H */
