// -*- C++ -*-
//

#include "Reader.h"
#include "../common/SampleInfo.h"
#include "../common/TestException.h"
#include "../common/TestSupport.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Serializer.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"


Reader::Reader(::DDS::DataReader_ptr reader,
               int use_take,
               int num_reads_per_thread,
               int multiple_instances,
               int reader_id)
: reader_ (::DDS::DataReader::_duplicate (reader)),
  use_take_ (use_take),
  num_reads_per_thread_ (num_reads_per_thread),
  multiple_instances_ (multiple_instances),
  reader_id_ (reader_id)
{
}

void
Reader::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Reader::start \n")));

  try
  {

    ::Xyz::FooDataReader_var foo_dr
      = ::Xyz::FooDataReader::_narrow(reader_.in ());
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) ::Xyz::FooDataReader::_narrow failed.\n")));
      throw TestException() ;
    }

    char action[5] ;
    if (use_take_)
    {
      ACE_OS::strcpy(action, (const char*)"take") ;
    }
    else
    {
      ACE_OS::strcpy(action, (const char*)"read") ;
    }
    for (int i = 0; i< num_reads_per_thread_; i ++)
    {
      ::Xyz::Foo foo;
      ::DDS::SampleInfo si ;

      DDS::ReturnCode_t status  ;
      if (use_take_)
      {
        status = foo_dr->take_next_sample(foo, si) ;
      }
      else
      {
        status = foo_dr->read_next_sample(foo, si) ;
      }

      if (status == ::DDS::RETCODE_OK)
      {
        ACE_OS::printf ("%s foo.x = %f foo.y = %f, foo.key = %d\n",
                        action, foo.x, foo.y, foo.key);

        PrintSampleInfo(si) ;
      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_OS::printf ("%s foo: ::DDS::RETCODE_NO_DATA\n", action) ;
        throw TestException() ;
      }
      else
      {
        ACE_OS::printf ("read  foo: Error: %d\n", status) ;
        throw TestException() ;
      }

    }

    if (use_take_)
    {
      //  Verify the samples were actually taken
      //
      ::Xyz::FooSeq foo(1) ;
      ::DDS::SampleInfoSeq si(1) ;

      ACE_OS::printf ("release: %d\n", foo.release()) ;

      DDS::ReturnCode_t status ;
      status = foo_dr->read(foo, si,
                  1,
                  ::DDS::READ_SAMPLE_STATE | ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;

      if (status != ::DDS::RETCODE_NO_DATA)
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Data found when all of it should have been taken.\n")));
        throw TestException() ;
      }
      else
      {
        ACE_OS::printf ("read foo: ::DDS::RETCODE_NO_DATA, as expected\n") ;
      }
    }
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
    throw TestException() ;
  }
}


void
Reader::start1 ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Reader::start1 \n")));

  try
  {

    ::Xyz::FooDataReader_var foo_dr
      = ::Xyz::FooDataReader::_narrow(reader_.in ());
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) ::Xyz::FooDataReader::_narrow failed.\n")));
      throw TestException() ;
    }

    char action[14] ;
    if (use_take_)
    {
      ACE_OS::strcpy(action, "take_instance") ;
    }
    else
    {
      ACE_OS::strcpy(action, "read_instance") ;
    }

    ::Xyz::FooSeq foo(num_reads_per_thread_) ;
    ::DDS::SampleInfoSeq si(num_reads_per_thread_) ;
    ::Xyz::Foo key_holder ;

    ::DDS::InstanceHandle_t handle(::DDS::HANDLE_NIL) ;
    DDS::ReturnCode_t status  ;
    int num_print(0) ;
    if (use_take_)
    {
      status = foo_dr->take_next_instance(foo, si,
                  1,
                  handle,
                  ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;

      if (status != ::DDS::RETCODE_OK)
      {
        ACE_OS::printf ("%s foo: returned %d\n", action, status) ;
        throw TestException() ;
      }

      ACE_OS::printf ("%s foo.x = %f foo.y = %f, foo.key = %d\n",
                      action, foo[0].x, foo[0].y, foo[0].key);

      PrintSampleInfo(si[0]);

      status = foo_dr->get_key_value(key_holder, si[0].instance_handle) ;
      TEST_CHECK(status == ::DDS::RETCODE_OK);

      PrintSampleInfo(si[0]) ;

      num_print++ ;

      if ((num_reads_per_thread_ > 1) || multiple_instances_)
      {
        handle = si[0].instance_handle ;
        status = foo_dr->take_instance(foo, si, num_reads_per_thread_,
                  handle,
                  ::DDS::READ_SAMPLE_STATE | ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;

        if (status == ::DDS::RETCODE_NO_DATA)
        {
          status = ::DDS::RETCODE_OK ;
          while (status != ::DDS::RETCODE_NO_DATA)
          {
            handle = ::DDS::HANDLE_NIL ;
            status = foo_dr->take_next_instance(foo, si,
                  1,
                  handle,
                  ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;
            if (status == ::DDS::RETCODE_OK)
            {
              handle = si[0].instance_handle ;
              ACE_OS::printf ("%s foo.x = %f foo.y = %f, foo.key = %d\n",
                      action, foo[0].x, foo[0].y, foo[0].key);

              PrintSampleInfo(si[0]);

              // Regression Test for https://github.com/objectcomputing/OpenDDS/issues/592
              status = foo_dr->get_key_value(key_holder, ::DDS::HANDLE_NIL) ;
              TEST_CHECK(status == ::DDS::RETCODE_BAD_PARAMETER);

              status = foo_dr->get_key_value(key_holder, si[0].instance_handle) ;
              TEST_CHECK(status == ::DDS::RETCODE_OK);
              ACE_OS::printf (
                  "get_key_value: handle = %d foo.x = %f foo.y = %f, foo.key = %d\n",
                  si[0].instance_handle,
                  key_holder.x,
                  key_holder.y,
                  key_holder.key);
              num_print++ ;
            }
            else if (status != ::DDS::RETCODE_NO_DATA)
            {
              ACE_OS::printf ("%s foo: returned %d\n", action, status) ;
              throw TestException() ;
            }
          }
        }
        else if (status != ::DDS::RETCODE_OK)
        {
          ACE_OS::printf ("%s foo: returned %d\n", action, status) ;
          throw TestException() ;
        }
      }
    }
    else
    {
      status = foo_dr->read_next_instance(foo, si,
                  1,
                  handle,
                  ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;

      if (status != ::DDS::RETCODE_OK)
      {
        ACE_OS::printf ("%s foo: returned %d\n", action, status) ;
        throw TestException() ;
      }

      ACE_OS::printf ("%s foo.x = %f foo.y = %f, foo.key = %d\n",
                      action, foo[0].x, foo[0].y, foo[0].key);

      PrintSampleInfo(si[0]);

      // Regression Test for https://github.com/objectcomputing/OpenDDS/issues/592
      status = foo_dr->get_key_value(key_holder, ::DDS::HANDLE_NIL) ;
      TEST_CHECK(status == ::DDS::RETCODE_BAD_PARAMETER);

      status = foo_dr->get_key_value(key_holder, si[0].instance_handle) ;
      TEST_CHECK(status == ::DDS::RETCODE_OK);

      ACE_OS::printf (
          "get_key_value: handle = %d foo.x = %f foo.y = %f, foo.key = %d\n",
          si[0].instance_handle, key_holder.x, key_holder.y, key_holder.key);
      num_print++ ;

      if ((num_reads_per_thread_ > 1) || multiple_instances_)
      {
        handle = si[0].instance_handle ;
        status = foo_dr->read_instance(foo, si, num_reads_per_thread_,
                  handle,
                  ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;

        if (status == ::DDS::RETCODE_NO_DATA)
        {
          status = ::DDS::RETCODE_OK ;
          while (status != ::DDS::RETCODE_NO_DATA)
          {
            handle = ::DDS::HANDLE_NIL ;
            status = foo_dr->read_next_instance(foo, si,
                  1,
                  handle,
                  ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;
            if (status == ::DDS::RETCODE_OK)
            {
              handle = si[0].instance_handle ;
              ACE_OS::printf ("%s foo.x = %f foo.y = %f, foo.key = %d\n",
                      action, foo[0].x, foo[0].y, foo[0].key);

              PrintSampleInfo(si[0]);

              // Regression Test for https://github.com/objectcomputing/OpenDDS/issues/592
              status = foo_dr->get_key_value(key_holder, ::DDS::HANDLE_NIL) ;
              TEST_CHECK(status == ::DDS::RETCODE_BAD_PARAMETER);

              status = foo_dr->get_key_value(key_holder, si[0].instance_handle) ;
              TEST_CHECK(status == ::DDS::RETCODE_OK);
              ACE_OS::printf (
                  "get_key_value: handle = %d foo.x = %f foo.y = %f, foo.key = %d\n",
                  si[0].instance_handle,
                  key_holder.x,
                  key_holder.y,
                  key_holder.key);
              num_print++ ;
            }
            else if (status != ::DDS::RETCODE_NO_DATA)
            {
              ACE_OS::printf ("%s foo: returned %d\n", action, status) ;
              throw TestException() ;
            }
          }
        }
        else if (status != ::DDS::RETCODE_OK)
        {
          ACE_OS::printf ("%s foo: returned %d\n", action, status) ;
          throw TestException() ;
        }
      }
    }

    int end_idx(num_reads_per_thread_ - num_print) ;

    for (int i = 0; i< end_idx ; i ++)
    {
      ACE_OS::printf ("%s foo.x = %f foo.y = %f, foo.key = %d\n",
                      action, foo[i].x, foo[i].y, foo[i].key);

      PrintSampleInfo(si[i]);
    }

    if (use_take_)
    {
      //  Verify the damples were actually taken
      //
      ::Xyz::FooSeq foo(1) ;
      ::DDS::SampleInfoSeq si(1) ;

      DDS::ReturnCode_t status ;
      status = foo_dr->read(foo, si,
                  1,
                  ::DDS::READ_SAMPLE_STATE | ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;

      if (status != ::DDS::RETCODE_NO_DATA)
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Data found when all of it should have been taken.\n")));
        throw TestException() ;
      }
      else
      {
        ACE_OS::printf ("read foo: ::DDS::RETCODE_NO_DATA, as expected\n") ;
      }
    }
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
    throw TestException() ;
  }
}


void
Reader::start2 ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Reader::start2 \n")));

  try
  {

    ::Xyz::FooDataReader_var foo_dr
      = ::Xyz::FooDataReader::_narrow(reader_.in ());
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) ::Xyz::FooDataReader::_narrow failed.\n")));
      throw TestException() ;
    }

    ::Xyz::FooSeq foo;
    ::DDS::SampleInfoSeq si ;
    DDS::ReturnCode_t status  ;

    status = foo_dr->read(foo, si,
                              num_reads_per_thread_,
                              ::DDS::NOT_READ_SAMPLE_STATE,
                              ::DDS::ANY_VIEW_STATE,
                              ::DDS::ANY_INSTANCE_STATE) ;


    if (status == ::DDS::RETCODE_OK)
    {
      for (int i = 0 ; i <  num_reads_per_thread_ ; i++)
      {
        ACE_OS::printf ("foo[%d]: x = %f y = %f, key = %d\n",
                        i, foo[i].x, foo[i].y, foo[i].key);

        PrintSampleInfo(si[i]) ;
      }

      status = foo_dr->return_loan(foo, si) ;
      if (status != ::DDS::RETCODE_OK)
      {
        ACE_OS::printf ("return_loan: Error %d\n", status) ;
        throw TestException() ;
      }
    }
    else if (status == ::DDS::RETCODE_NO_DATA)
    {
      ACE_OS::printf ("read foo: ::DDS::RETCODE_NO_DATA\n") ;
      throw TestException() ;
    }
    else
    {
      ACE_OS::printf ("read  foo: Error: %d\n", status) ;
      throw TestException() ;
    }
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
    throw TestException() ;
  }
}


long
Reader::reader_id () const
{
  return reader_id_;
}

