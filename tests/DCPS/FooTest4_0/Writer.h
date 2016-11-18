// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsSubscriptionC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

class Writer
{
public:

  Writer (::DDS::DomainParticipant_ptr dp,
          ::DDS::Topic_ptr topic,
          int history_depth,
          int max_samples_per_instance);

  ~Writer() ;

  void test1 ();
  void test2 ();
  void test3 ();
  void test4 ();
  void test5 ();
  void test6 ();

private:

  int init_transport () ;
  void write (char message_id, const ::Xyz::Foo& foo);

  ::DDS::DomainParticipant_var dp_;
  ::DDS::Publisher_var pub_;
  ::DDS::DataWriter_var dw_;
  ::Xyz::FooDataWriter_var foo_dw_;
};

#endif /* READER_H */
