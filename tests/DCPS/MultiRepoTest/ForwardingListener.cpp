// -*- C++ -*-
//
#include "ForwardingListener.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/Service_Participant.h"

template <class DT, class DT_seq, class DR, class DR_ptr, class DR_var>
int read (::DDS::DataReader_ptr reader, DT& foo, bool& valid_data)
{
  try
  {
    DR_var foo_dr
      = DR::_narrow(reader);
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::read - _narrow failed.\n")));
      throw BadReaderException() ;
    }

    ::DDS::SampleInfo si ;

    DDS::ReturnCode_t const status = foo_dr->read_next_sample(foo, si) ;

    if (status == ::DDS::RETCODE_OK)
    {
      valid_data = si.valid_data;
      if (si.valid_data == 1)
      {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::read %X foo.x = %f foo.y = %f, foo.data_source = %d\n"),
          reader, foo.x, foo.y, foo.data_source));
      }
      else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is disposed\n")));
      }
      else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is unregistered\n")));
      }
      else
      {
        ACE_ERROR ((LM_ERROR, "(%P|%t) ForwardingListenerImpl::read:"
          " received unknown instance state %d\n", si.instance_state));
      }
    }
    else if (status == ::DDS::RETCODE_NO_DATA)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) %T ERROR: ForwardingListenerImpl::reader received ::DDS::RETCODE_NO_DATA!\n")),
        -1);
    }
    else
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) %T ERROR: ForwardingListenerImpl::read status==%d\n"), status),
        -1);
    }
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("(%P|%t) %T ForwardingListenerImpl::read - ");
    return -1;
  }

  return 0;
}


ForwardingListenerImpl::ForwardingListenerImpl(
  OpenDDS::DCPS::Discovery::RepoKey repo
) : samples_( 0),
    condition_( this->lock_),
    complete_ (false),
    repo_( repo)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::ForwardingListenerImpl Repo[ %C]\n"),
             this->repo_.c_str()
  ));
}

ForwardingListenerImpl::~ForwardingListenerImpl (void)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::~ForwardingListenerImpl Repo[ %C] ")
      ACE_TEXT("after %d samples\n"),
      this->repo_.c_str(),
      this->samples_
    ));
  }

/// Writer to forward data on.
void ForwardingListenerImpl::dataWriter(
  ::DDS::DataWriter_ptr writer
)
{
  this->dataWriter_ = ::DDS::DataWriter::_duplicate( writer);
}

void
ForwardingListenerImpl::waitForCompletion()
{
  ACE_GUARD (ACE_SYNCH_MUTEX, g, this->lock_);
  while (!this->complete_)
    {
      this->condition_.wait();
    }
}

void ForwardingListenerImpl::on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_requested_deadline_missed Repo[ %C]\n"),
      this->repo_.c_str()
    ));
  }

void ForwardingListenerImpl::on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_requested_incompatible_qos Repo[ %C]\n"),
      this->repo_.c_str()
    ));
  }

void ForwardingListenerImpl::on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_liveliness_changed Repo[ %C]\n"),
      this->repo_.c_str()
    ));
  }

void ForwardingListenerImpl::on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_subscription_matched Repo[ %C] \n"),
      this->repo_.c_str()
    ));
  }

  void ForwardingListenerImpl::on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_sample_rejected Repo[ %C] \n"),
      this->repo_.c_str()
    ));
  }

  void ForwardingListenerImpl::on_data_available(
    ::DDS::DataReader_ptr reader
  )
  {
    ::Xyz::FooNoKey foo;
    bool valid_data = false;
    int ret = read <Xyz::FooNoKey,
        ::Xyz::FooNoKeySeq,
        ::Xyz::FooNoKeyDataReader,
        ::Xyz::FooNoKeyDataReader_ptr,
        ::Xyz::FooNoKeyDataReader_var> (reader, foo, valid_data);

    if (ret != 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_data_available Repo[ %C] read failed.\n"),
        this->repo_.c_str()
      ));

    } else if( CORBA::is_nil( this->dataWriter_.in())) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_data_available Repo[ %C] - bit bucket reached. \n"),
        this->repo_.c_str()
      ));
      // The bit bucket is done processing when the answer is received.
      if(valid_data && foo.data_source == 42) {
        ACE_GUARD (ACE_SYNCH_MUTEX, g, this->lock_);
        this->complete_ = true;
        this->condition_.signal();
      }

    } else if(valid_data && foo.data_source == 30) {
      // Signal that we are done once we receive a disconnect message.
      // We use the data_source member as a command value.
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_data_available Repo[ %C] - termination command received. \n"),
        this->repo_.c_str()
      ));
      ACE_GUARD (ACE_SYNCH_MUTEX, g, this->lock_);
      this->complete_ = true;
      this->condition_.signal();

    } else {
      // This is narrowed to forward each sample to avoid hoisting the
      // type up into the header and creating more dependencies.  If this
      // were a performance application, that might be a better option.
      ::Xyz::FooNoKeyDataWriter_var fooWriter
        = ::Xyz::FooNoKeyDataWriter::_narrow( this->dataWriter_.in());
      if( CORBA::is_nil( fooWriter.in())) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_data_available Repo[ %C] ")
          ACE_TEXT("failed to narrow writer to forward a sample with.\n"),
          this->repo_.c_str()
        ));

      } else if (valid_data)
      {
        // Modify the data as it passes, just to prove it has been here.
        foo.x += 1.0;
        foo.y += 2.0;

        // Go ahead and forward the data.
        if( ::DDS::RETCODE_OK != fooWriter->write( foo, ::DDS::HANDLE_NIL)) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_data_available Repo[ %C] ")
            ACE_TEXT("failed to forward a sample.\n"),
            this->repo_.c_str()
          ));
        }
      }
    }
  }

  void ForwardingListenerImpl::on_sample_lost(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T ForwardingListenerImpl::on_sample_lost Repo[ %C] \n"),
      this->repo_.c_str()
    ));
  }
