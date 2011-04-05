// -*- C++ -*-
//
// $Id$
#ifndef SIMPLEDATAWRITER_H
#define SIMPLEDATAWRITER_H

#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/DataSampleList.h"
#include "ace/Synch.h"
#include "ace/Condition_T.h"

class SimplePublisher;

class SimpleDataWriter : public OpenDDS::DCPS::TransportSendListener
{
  public:

    SimpleDataWriter();
    ~SimpleDataWriter();

    void init(OpenDDS::DCPS::RepoId pub_id);
    int  run(SimplePublisher* publisher, unsigned num_messages);

    void transport_lost();

    void data_delivered(const OpenDDS::DCPS::DataSampleListElement* sample);
    void data_dropped(const OpenDDS::DCPS::DataSampleListElement* sample,
                      bool dropped_by_transport = false);

    int delivered_test_message();


  private:

    typedef ACE_Recursive_Thread_Mutex LockType;
    typedef ACE_Guard<LockType>        GuardType;
    typedef ACE_Condition<LockType>    ConditionType;

    void obtain_element(SimplePublisher* publisher);
    unsigned release_element(OpenDDS::DCPS::DataSampleListElement* sample);

    LockType lock_;

    OpenDDS::DCPS::RepoId pub_id_;

    unsigned num_sent_;
    unsigned num_to_send_;
    unsigned num_delivered_;

    OpenDDS::DCPS::DataSampleListElement* element_;

    ConditionType condition_;

    OpenDDS::DCPS::DataSampleListElementAllocator* allocator_;
    OpenDDS::DCPS::TransportSendElementAllocator* trans_allocator_;
};

#endif  /* SIMPLEDATAWRITER_H */
