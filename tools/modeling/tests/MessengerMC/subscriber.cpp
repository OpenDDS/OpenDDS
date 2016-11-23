
#include <ace/Log_Msg.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "fs_signal.h"

#include "model/MessengerMCTraits.h"
#include <model/NullReaderListener.h>

class ReaderListener : public OpenDDS::Model::NullReaderListener {

  public:
    ReaderListener(bool& complete_flag);
  private:
  virtual void on_data_available(DDS::DataReader_ptr reader);

    bool& _complete;
};

ReaderListener::ReaderListener(bool& complete_flag) : _complete(complete_flag) {
}

// START OF EXISTING MESSENGER EXAMPLE LISTENER CODE

void
ReaderListener::on_data_available(DDS::DataReader_ptr reader)
{
  data1::MessageDataReader_var reader_i =
    data1::MessageDataReader::_narrow(reader);

  if (CORBA::is_nil(reader_i.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  data1::Message message;
  DDS::SampleInfo info;

  DDS::ReturnCode_t error = reader_i->take_next_sample(message, info);

  if (error == DDS::RETCODE_OK) {
    std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
    std::cout << "SampleInfo.instance_state = " << info.instance_state << std::endl;

    if (info.valid_data) {
      std::cout << "Message: subject    = " << message.subject.in() << std::endl
                << "         subject_id = " << message.subject_id   << std::endl
                << "         from       = " << message.from.in()    << std::endl
                << "         count      = " << message.count        << std::endl
                << "         text       = " << message.text.in()    << std::endl;

      if (message.count == 9) {
        // Signal completion to publisher
        FileSystemSignal(2).signal();
        _complete = true;
      }
    }

  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" take_next_sample failed!\n")));
  }
}

// END OF EXISTING MESSENGER EXAMPLE LISTENER CODE

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  bool complete = false;
  try {
    OpenDDS::Model::Application application(argc, argv);
    MessengerMC::DefaultMessengerMCType model(application, argc, argv);

    using OpenDDS::Model::MessengerMC::Elements;

    DDS::DataReader_var reader = model.reader( Elements::DataReaders::reader);

    DDS::DataReaderListener_var listener(new ReaderListener(complete));
    reader->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::MessageDataReader_var reader_i =
      data1::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(reader_i.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
    }

    ACE_OS::sleep(2);

    std::cout << "sub signaling ready" << std::endl;
    // Signal readiness to publisher
    FileSystemSignal(1).signal();

    // END OF EXISTING MESSENGER EXAMPLE CODE

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;

  } catch( const std::exception& ex) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                      ACE_TEXT(" Exception caught: %C\n"),
                      ex.what()),
                     -1);
  }

  ACE_OS::sleep(5);

  std::cout << "sub exiting" << std::endl;
  return 0;
}
