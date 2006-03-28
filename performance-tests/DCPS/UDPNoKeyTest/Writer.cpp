// -*- C++ -*-
//
// $Id$
#include "Writer.h"
#include "TestException.h"
#include "../TypeNoKeyBounded/Pt128TypeSupportC.h"
#include "../TypeNoKeyBounded/Pt512TypeSupportC.h"
#include "../TypeNoKeyBounded/Pt2048TypeSupportC.h"
#include "../TypeNoKeyBounded/Pt8192TypeSupportC.h"
#include "../TypeNoKeyBounded/Pt128TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt512TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt2048TypeSupportImpl.h"
#include "../TypeNoKeyBounded/Pt8192TypeSupportImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "ace/OS_NS_unistd.h"



// throttle by spinning;  ACE_OS::sleep() minimum sleep is 10 miliseconds
#define THROTTLE \
  if (throttle_factor_) { \
    double ttt1 = 13.0L; \
    for (unsigned ti=0; ti < throttle_factor_; ti++) { \
      ttt1 = (ti * 5.0L)/ttt1; \
    } \
  }




template<class T, class W, class W_var, class W_ptr, class Wimpl>
void write (long id,
            int size,
            int num_messages,
            unsigned throttle_factor_,
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
    = W::_narrow(writer ACE_ENV_ARG_PARAMETER);
  ACE_ASSERT (! CORBA::is_nil (pt_dw.in ()));

  Wimpl* pt_servant =
    ::TAO::DCPS::reference_to_servant< Wimpl, W_ptr>
            (pt_dw.in ()  ACE_ENV_SINGLE_ARG_PARAMETER);

  //SHH remove this kludge when the transport is fixed.
  ACE_OS::sleep(2); // ensure that the connection has been fully established
  ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("%T (%P|%t) Writer::svc starting to write.\n")));

  ::DDS::InstanceHandle_t handle 
      = pt_dw->_cxx_register (data ACE_ENV_ARG_PARAMETER);

  {
  for (int i = 0; i < num_messages; i ++)
    {
      data.sequence_num = i;
      pt_servant->write(data, 
                        handle 
                        ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      THROTTLE
    }
  }

  for (int j = 0; j < 500; j ++)
    {
      data.sequence_num = -1;
      pt_servant->write(data, 
                        handle 
                        ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      // throttle twice to slow the rate to ensure one gets sent.
      THROTTLE
      THROTTLE
    }
}





Writer::Writer(::DDS::DataWriter_ptr writer, 
               int num_messages,
               int data_size, 
               int writer_id,
               int num_readers,
               unsigned throttle_factor)
: writer_ (::DDS::DataWriter::_duplicate (writer)),
  num_messages_ (num_messages),
  data_size_ (data_size),
  num_floats_per_sample_ (1 << data_size),
  writer_id_ (writer_id),
  num_readers_ (num_readers),
  finished_sending_ (false),
  throttle_factor_(throttle_factor)
{
}

void 
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT(" %P|%t Writer::start \n")));
  if (activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1) 
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT(" %P|%t Writer::start, ")
                ACE_TEXT ("%p."), 
                "activate")); 
    throw TestException ();
  }
}

void 
Writer::end () 
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT(" %P|%t Writer::end \n")));
  wait ();
}


int 
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT(" %P|%t Writer::svc begins samples with %d floats.\n"),
              1 << data_size_));

  ACE_TRY_NEW_ENV
  {
    int num_connected_subs = 0;
    ::DDS::InstanceHandleSeq handles;
    while (num_connected_subs != num_readers_)
      {
        ACE_OS::sleep(1);
        writer_->get_matched_subscriptions(handles);
        num_connected_subs = handles.length();
      }

    switch ( 1 << data_size_ )
    {

    case 128:
      {
        write < ::Xyz::Pt128,
               ::Mine::Pt128DataWriter,
               ::Mine::Pt128DataWriter_var,
               ::Mine::Pt128DataWriter_ptr,
               ::Mine::Pt128DataWriterImpl>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  throttle_factor_,
                  writer_.in ());
      }
      break;

    case 512:
      {
        write < ::Xyz::Pt512,
               ::Mine::Pt512DataWriter,
               ::Mine::Pt512DataWriter_var,
               ::Mine::Pt512DataWriter_ptr,
               ::Mine::Pt512DataWriterImpl>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  throttle_factor_,
                  writer_.in ());
      }
      break;

    case 2048:
      {
        write < ::Xyz::Pt2048,
               ::Mine::Pt2048DataWriter,
               ::Mine::Pt2048DataWriter_var,
               ::Mine::Pt2048DataWriter_ptr,
               ::Mine::Pt2048DataWriterImpl>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  throttle_factor_,
                  writer_.in ());
      }
      break;

    case 8192:
      {
        write < ::Xyz::Pt8192,
               ::Mine::Pt8192DataWriter,
               ::Mine::Pt8192DataWriter_var,
               ::Mine::Pt8192DataWriter_ptr,
               ::Mine::Pt8192DataWriterImpl>
                 (writer_id_,
                  data_size_,
                  num_messages_,
                  throttle_factor_,
                  writer_.in ());
      }
      break;

    default:
      ACE_DEBUG((LM_ERROR,"ERROR: bad data size %d\n", data_size_));
      return 1;
      break;
    };


  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in svc:");
  }
  ACE_ENDTRY;

  finished_sending_ = true;

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT(" %P|%t Writer::svc finished.\n")));
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

