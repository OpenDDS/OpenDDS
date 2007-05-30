// -*- C++ -*-
//
// $Id$
// -*- C++ -*-
//
// $Id$
#include "Writer.h"
#include "TestException.h"
#include "tests/DCPS/common/TestSupport.h"

Writer::Writer(::DDS::DataWriter_ptr writer,
               int num_thread_to_write,
               int num_writes_per_thread,
               int writer_id)
: writer_ (::DDS::DataWriter::_duplicate(writer)),
  num_thread_to_write_ (num_thread_to_write),
  num_writes_per_thread_ (num_writes_per_thread),
  writer_id_ (writer_id),
  handle_ (-1)
{
  registered_foo_.a_long_value = (CORBA::Long) (ACE_OS::thr_self ());
  registered_foo_.sample_sequence = -1;
  registered_foo_.handle_value = -1;
  registered_foo_.writer_id = writer_id_;
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Writer::start \n")));

  // Register the instance without key then use this handle
  // to verify other registrations always return same handle.
  foo_dw_ = ::Xyz::FooDataWriter::_narrow(writer_.in ());
  TEST_CHECK (! CORBA::is_nil (foo_dw_.in ()));

  handle_ = foo_dw_->_cxx_register (registered_foo_);

  if (activate (THR_NEW_LWP | THR_JOINABLE, num_thread_to_write_) == -1)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) Writer::start, ")
                ACE_TEXT ("%p."),
                "activate"));
    throw TestException ();
  }
}

void
Writer::end ()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::end \n")));
  wait ();
}


int
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc \n")));

  try
  {
    ::Xyz::Foo foo;
    // Use the thread id as a_long_value which is used as key in the
    // FooTest3.
    foo.a_long_value = (CORBA::Long) (ACE_OS::thr_self ());
    foo.sample_sequence = -1;
    foo.handle_value = -1;
    foo.writer_id = writer_id_;


    for (int i = 0; i< num_writes_per_thread_; i ++)
    {
      ::DDS::InstanceHandle_t handle
        = foo_dw_->_cxx_register (foo);

      // Any registration with different a_long_value should always
      // return the same handle.
      TEST_CHECK (handle_ == handle);

      foo.handle_value = handle;

      // The sequence number will be increased after the insert.
      TEST_CHECK (data_map_.insert (handle, foo) == 0);

      // Calling get_key_value does not make sense since the Foo type has
      // no key. But we call it to verify if the instance registered is
      // always the same.
      ::Xyz::Foo key_holder;
      ::DDS::ReturnCode_t ret
        = foo_dw_->get_key_value(key_holder, handle);

      // check for equality
      TEST_CHECK(ret == ::DDS::RETCODE_OK);
      TEST_CHECK (key_holder.a_long_value == registered_foo_.a_long_value);
      TEST_CHECK(key_holder.sample_sequence == registered_foo_.sample_sequence);
      TEST_CHECK(key_holder.handle_value == registered_foo_.handle_value);
      TEST_CHECK(key_holder.writer_id == registered_foo_.writer_id);

      foo_dw_->write(foo,
                    handle);
    }
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
  }

  return 0;
}

long
Writer::writer_id () const
{
  return writer_id_;
}


InstanceDataMap&
Writer::data_map ()
{
  return data_map_;
}

