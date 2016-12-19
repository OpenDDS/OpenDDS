
#include <ace/Log_Msg.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/ExchangeTraits.h"
#include <model/NullReaderListener.h>

class ReaderListener : public OpenDDS::Model::NullReaderListener {
  public:
    ReaderListener(bool& disposed) : _disposed(disposed)
    { }
  virtual void on_data_available(
    DDS::DataReader_ptr reader);
  private:
    bool& _disposed;
};

// START OF EXISTING MESSENGER EXAMPLE LISTENER CODE

void
ReaderListener::on_data_available(DDS::DataReader_ptr reader)
{
  TMQDataReader_var reader_i =
    TMQDataReader::_narrow(reader);

  if (CORBA::is_nil(reader_i.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  TMQ message;
  DDS::SampleInfo info;

  while (true) {
    DDS::ReturnCode_t error = reader_i->take_next_sample(message, info);
    std::cout << "take status = " << error << std::endl;
    if (error == DDS::RETCODE_OK) {
      std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
      std::cout << "SampleInfo.instance_state = " << info.instance_state << std::endl;

      if (info.valid_data) {
        std::cout << "TMQ:   symbol     = " << message.symbol.in() << std::endl
                  << "       ask        = " << message.ask         << std::endl
                  << "       price      = " << message.price       << std::endl
                  << "       bid        = " << message.bid         << std::endl;

      } else if (info.instance_state & DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        std::cout << "subscriber detected instance disposed" << std::endl;
        _disposed = true;
        break;
      }
    } else if (error != DDS::RETCODE_NO_DATA) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                 ACE_TEXT(" take_next_sample failed!\n")));
      break;
    } else {
      if (info.instance_state & DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        std::cout << "subscriber detected instance disposed" << std::endl;
        _disposed = true;
      }
      break;
    }
  }
  std::cout << "on_data_available, exiting" << std::endl;
}

// END OF EXISTING MESSENGER EXAMPLE LISTENER CODE

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  bool disposed = false;
  try {
    std::cout << "subscriber running" << std::endl;
    OpenDDS::Model::Application application(argc, argv);
    ExchangeLib::DefaultExchangeType model(application, argc, argv);

    using OpenDDS::Model::ExchangeLib::Elements;

    DDS::TopicDescription_var tdv(model.topic(Elements::Participants::FeedConsumption,
                                             Elements::Topics::ExchangeLib__Trades));
    DDS::TopicDescription_var qdv(model.topic(Elements::Participants::FeedConsumption,
                                             Elements::Topics::ExchangeLib__Quotes));
    DDS::DataReader_var reader = model.reader(Elements::DataReaders::MatchReader);

    DDS::DataReaderListener_var listener(new ReaderListener(disposed));
    reader->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    TMQDataReader_var reader_i = TMQDataReader::_narrow(reader);

    if (CORBA::is_nil(reader_i.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
    }

    while (!disposed) {
      ACE_OS::sleep(1);
    }
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

  std::cout << "subscriber exiting" << std::endl;
  return 0;
}
