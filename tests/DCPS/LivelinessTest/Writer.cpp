// -*- C++ -*-
//
#include "Writer.h"
#include "../common/TestException.h"
#include "../common/TestSupport.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportC.h"
#include "ace/OS_NS_unistd.h"

const int default_key = 101010;


Writer::Writer(::DDS::DataWriter_ptr writer,
               int num_thread_to_write,
               int num_writes_per_thread)
: writer_ (::DDS::DataWriter::_duplicate (writer)),
  num_thread_to_write_ (num_thread_to_write),
  num_writes_per_thread_ (num_writes_per_thread),
  multiple_instances_(0),
  finished_sending_ (false)
{
}

int
Writer::run_test (int pass)
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::run_test begins.\n")));

  try
  {
    finished_sending_ = false;

    ::Xyz::Foo foo;
    //foo.key set below.
    foo.x = -1;
    foo.y = -1;

    foo.key = default_key;

    ::Xyz::FooDataWriter_var foo_dw
      = ::Xyz::FooDataWriter::_narrow(writer_.in ());
    TEST_CHECK (! CORBA::is_nil (foo_dw.in ()));

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Writer::run_test starting to write pass %d\n"),
              pass));

    ::DDS::InstanceHandle_t handle
        = foo_dw->register_instance(foo);

    for (int i = 0; i< num_writes_per_thread_; i ++)
    {

      foo.x = (float)i;
      foo.y = (float)(-pass) ;

      foo_dw->write(foo,
                    handle);
    }

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Writer::run_test done writing.\n")));

  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in run_test:");
  }

  finished_sending_ = true;

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::run_test finished.\n")));
  return 0;
}


bool
Writer::is_finished () const
{
  return finished_sending_;
}

