// -*- C++ -*-
//
#include "Writer.h"
#include "TestException.h"
#include "../TypeNoKeyBounded/PTDefTypeSupportC.h"
#include "../TypeNoKeyBounded/PTDefTypeSupportImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "ace/OS_NS_unistd.h"

template<class T, class W, class W_var, class W_ptr>
void write (long id,
            int size,
            int num_messages,
            ::DDS::DataWriter_ptr writer)
{
  T data;
  data.data_source = id;
  data.values.length(size);

  for (int i = 0; i < size; i ++)
  {
    data.values[i] = (float) i;
  }

  W_var pt_dw
    = W::_narrow(writer);
  ACE_ASSERT (! CORBA::is_nil (pt_dw.in ()));

  //SHH remove this kludge when the transport is fixed.
  ACE_OS::sleep(5); // ensure that the connection has been fully established
  ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));

  ::DDS::InstanceHandle_t handle
      = pt_dw->register_instance(data);

  {
  for (int i = 0; i < num_messages; i ++)
  {
    data.sequence_num = i;
    ::DDS::ReturnCode_t ret = pt_dw->write(data, handle);
    if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("write failed for msg_num %d\n"),
                           i ));
      }
  }
  }
}




Writer::Writer(::DDS::DataWriter_ptr writer,
               int num_messages,
               int data_size,
               int writer_id)
: writer_ (writer),
  num_messages_ (num_messages),
  data_size_ (data_size),
  writer_id_ (writer_id),
  finished_sending_ (false)
{
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Writer::start \n")));
  if (activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) Writer::start, %p.\n"),
                ACE_TEXT("activate")));
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
              ACE_TEXT("(%P|%t) Writer::svc begins samples with %d floats.\n"),
                data_size_));

  try
  {

    switch ( data_size_ )
    {

    case 128:
      {
        write < ::Xyz::Pt128,
               ::Xyz::Pt128DataWriter,
               ::Xyz::Pt128DataWriter_var,
               ::Xyz::Pt128DataWriter_ptr>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_);
      }
      break;

    case 512:
      {
        write < ::Xyz::Pt512,
               ::Xyz::Pt512DataWriter,
               ::Xyz::Pt512DataWriter_var,
               ::Xyz::Pt512DataWriter_ptr>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_);
      }
      break;

    case 2048:
      {
        write < ::Xyz::Pt2048,
               ::Xyz::Pt2048DataWriter,
               ::Xyz::Pt2048DataWriter_var,
               ::Xyz::Pt2048DataWriter_ptr>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_);
      }
      break;

    case 8192:
      {
        write < ::Xyz::Pt8192,
               ::Xyz::Pt8192DataWriter,
               ::Xyz::Pt8192DataWriter_var,
               ::Xyz::Pt8192DataWriter_ptr>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_);
      }
      break;

    default:
      ACE_DEBUG((LM_ERROR,"ERROR: bad data size %d\n", data_size_));
      return 1;
      break;
    };


  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
  }

  ::DDS::InstanceHandleSeq handles;
  while (!finished_sending_)
    {
      ACE_OS::sleep(1);
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        {
          finished_sending_ = true;
        }
    }

  // wait for the assocation to become fully established
  ACE_OS::sleep(1);

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc finished.\n")));
  return 0;
}

long
Writer::writer_id () const
{
  return writer_id_;
}


bool
Writer::is_finished () const
{
  return finished_sending_;
}

