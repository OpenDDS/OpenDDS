// -*- C++ -*-
//
#include "Writer.h"
#include "../common/TestException.h"
#include "../common/TestSupport.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportC.h"
#include "ace/OS_NS_unistd.h"

const int default_key = 101010;


Writer::Writer(::DDS::DataWriter_ptr writer)
: writer_ (::DDS::DataWriter::_duplicate (writer))
{
}

int
Writer::run_test (const ACE_Time_Value& duration)
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::run_test begins.\n")));

  ACE_Time_Value started = ACE_OS::gettimeofday ();
  unsigned int pass = 0;

  while(ACE_OS::gettimeofday() < started + duration)
  {
    ++pass;
    try
    {
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

      foo.x = 5.0;
      foo.y = (float)(pass) ;

      foo_dw->write(foo,
                    handle);

      ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t) %T Writer::run_test done writing.\n")));

      ACE_OS::sleep(1);
    }
    catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in run_test:");
    }
  }
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::run_test finished.\n")));
  return 0;
}
