// -*- C++ -*-
//
#include "Writer.h"

#include <tests/DCPS/common/TestException.h>
#include <tests/DCPS/common/TestSupport.h>
#include <tests/DCPS/FooType4/FooDefTypeSupportC.h>

#include <ace/OS_NS_unistd.h>

const int default_key = 101010;


Writer::Writer(DDS::DataWriter_ptr writer,
               int num_writes_per_thread)
: writer_(DDS::DataWriter::_duplicate (writer))
, num_writes_per_thread_(num_writes_per_thread)
{
}

int
Writer::run_test(int pass)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::run_test begins.\n")));

  ::Xyz::Foo foo;
  //foo.key set below.
  foo.x = -1;
  foo.y = -1;

  foo.key = default_key;

  ::Xyz::FooDataWriter_var foo_dw = ::Xyz::FooDataWriter::_narrow(writer_.in());

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) %T Writer::run_test starting to write pass %d\n"), pass));

  DDS::InstanceHandle_t handle = foo_dw->register_instance(foo);
  for (int i = 0; i< num_writes_per_thread_; ++i) {
    foo.x = (float)i;
    foo.y = (float)(-pass) ;
    foo_dw->write(foo, handle);
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T Writer::run_test done writing.\n")));

  return 0;
}
