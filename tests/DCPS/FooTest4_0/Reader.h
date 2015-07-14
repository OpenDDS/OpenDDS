// -*- C++ -*-
//
#ifndef READER_H
#define READER_H

#include "DataReaderListener.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "common.h"

class Reader
{
public:

  Reader (::DDS::DomainParticipant_ptr dp,
          int history_depth,
          int max_samples_per_instance);

  ~Reader();

  void read (const SampleInfoMap& si_map,
             ::DDS::SampleStateMask ss = ::DDS::ANY_SAMPLE_STATE,
             ::DDS::ViewStateMask vs = ::DDS::ANY_VIEW_STATE,
             ::DDS::InstanceStateMask is = ::DDS::ANY_INSTANCE_STATE);

private:
  int init_transport ();

  int max_samples_per_instance_ ;
  ::DDS::DomainParticipant_var dp_ ;
  ::DDS::Subscriber_var sub_;
};

#endif /* READER_H */
