
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include "model/ExchangeTraits.h"
#include <model/NullReaderListener.h>
#include <model/Sync.h>

class ReaderListener : public OpenDDS::Model::NullReaderListener {
  virtual void on_data_available(
    DDS::DataReader_ptr reader)
  ACE_THROW_SPEC((CORBA::SystemException));
};

// START OF EXISTING MESSENGER EXAMPLE LISTENER CODE

void
ReaderListener::on_data_available(DDS::DataReader_ptr reader)
ACE_THROW_SPEC((CORBA::SystemException))
{
  std::cout << "sub on_data_available" << std::endl;
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

  DDS::ReturnCode_t error = reader_i->take_next_sample(message, info);

  if (error == DDS::RETCODE_OK) {
    std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
    std::cout << "SampleInfo.instance_state = " << info.instance_state << std::endl;

    if (info.valid_data) {
      std::cout << "TMQ:   symbol     = " << message.symbol.in() << std::endl
                << "       ask        = " << message.ask         << std::endl
                << "       price      = " << message.price       << std::endl
                << "       bid        = " << message.bid         << std::endl;

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
  try {
    std::cout << "subscriber running" << std::endl;
    OpenDDS::Model::Application application(argc, argv);
    ExchangeLib::DefaultExchangeType model(application, argc, argv);

    using OpenDDS::Model::ExchangeLib::Elements;

    DDS::TopicDescription_var tdv(model.topic(Elements::Participants::FeedConsumption,
                                             Elements::Topics::Trades));
    DDS::TopicDescription_var qdv(model.topic(Elements::Participants::FeedConsumption,
                                             Elements::Topics::Quotes));
    DDS::DataReader_var reader = model.reader(Elements::DataReaders::MatchReader);

    DDS::DataReaderListener_var listener(new ReaderListener);
    reader->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    TMQDataReader_var reader_i = TMQDataReader::_narrow(reader);

    if (CORBA::is_nil(reader_i.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
    }

    //OpenDDS::Model::ReaderSync rs(reader);
    ACE_OS::sleep(5);
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
