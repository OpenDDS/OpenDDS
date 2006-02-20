// -*- C++ -*-
//
// $Id$
#ifndef SIMPLEDATAWRITER_H
#define SIMPLEDATAWRITER_H

#include  "dds/DCPS/transport/framework/TransportSendListener.h"
#include  "dds/DCPS/Definitions.h"
#include  "dds/DCPS/DataSampleList.h"
#include  "ace/Synch.h"
#include  "ace/Condition_T.h"

class SimplePublisher;

class SimpleDataWriter : public TAO::DCPS::TransportSendListener
{
  public:

    SimpleDataWriter();
    ~SimpleDataWriter();

    void init(TAO::DCPS::RepoId pub_id);
    int  run(SimplePublisher* publisher, unsigned num_messages);

    void transport_lost();

    virtual void data_delivered(TAO::DCPS::DataSampleListElement* sample);
    virtual void data_dropped(TAO::DCPS::DataSampleListElement* sample);

    int delivered_test_message();


  private:

    typedef ACE_Recursive_Thread_Mutex LockType;
    typedef ACE_Guard<LockType>        GuardType;
    typedef ACE_Condition<LockType>    ConditionType;

    void obtain_element(SimplePublisher* publisher);
    unsigned release_element(TAO::DCPS::DataSampleListElement* sample);

    LockType lock_;

    TAO::DCPS::RepoId pub_id_;

    unsigned num_sent_;
    unsigned num_to_send_;
    unsigned num_delivered_;

    TAO::DCPS::DataSampleListElement* element_;

    ConditionType condition_;

    TAO::DCPS::DataSampleListElementAllocator* allocator_;
    TAO::DCPS::TransportSendElementAllocator* trans_allocator_;
};

#endif  /* SIMPLEDATAWRITER_H */
