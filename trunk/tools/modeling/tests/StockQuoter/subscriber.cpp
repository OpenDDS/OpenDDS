
#include <model/Sync.h>
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/StockQuoterTraits.h"
#include <model/NullReaderListener.h>
#include <model/Sync.h>

class ReaderListener : public OpenDDS::Model::NullReaderListener {
  public:
    ReaderListener(OpenDDS::Model::ReaderCondSync& rcs) : rcs_(rcs),
        count_(0) {}
    virtual void on_data_available(DDS::DataReader_ptr reader);
  private:
    OpenDDS::Model::ReaderCondSync& rcs_;
    int count_;

};

// START OF EXISTING MESSENGER EXAMPLE LISTENER CODE

void
ReaderListener::on_data_available(DDS::DataReader_ptr reader)
{
  data1::QuoteDataReader_var reader_i =
    data1::QuoteDataReader::_narrow(reader);

  if (CORBA::is_nil(reader_i.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  data1::Quote msg;
  DDS::SampleInfo info;

  while (true) {
    DDS::ReturnCode_t error = reader_i->take_next_sample(msg, info);
    if (error == DDS::RETCODE_OK) {
      std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
      std::cout << "SampleInfo.instance_state = " << info.instance_state << std::endl;

      if (info.valid_data) {
        std::cout << "Message: ticker    = " << msg.ticker.in() << std::endl
                  << "         exchange  = " << msg.exchange.in()   << std::endl
                  << "         full_name = " << msg.full_name.in()    << std::endl;
        if (++count_ ==10) {
          rcs_.signal();
        }
      }
    } else {
      if (error != DDS::RETCODE_NO_DATA) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                   ACE_TEXT(" take_next_sample failed!\n")));
      }
      break;
    }
  }
}

// END OF EXISTING MESSENGER EXAMPLE LISTENER CODE

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    OpenDDS::Model::Application application(argc, argv);
    StockQuoter::DefaultStockQuoterType model(application, argc, argv);

    using OpenDDS::Model::StockQuoter::Elements;

    DDS::DataReader_var reader = model.reader(Elements::DataReaders::reader);

    ACE_SYNCH_MUTEX lock;
    ACE_Condition<ACE_SYNCH_MUTEX> condition(lock);
    OpenDDS::Model::ReaderCondSync rcs(reader, condition);
    DDS::DataReaderListener_var listener(new ReaderListener(rcs));
    reader->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // Call on_data_available in case there are samples which are waiting
    listener->on_data_available(reader);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::QuoteDataReader_var reader_i =
      data1::QuoteDataReader::_narrow(reader);

    if (CORBA::is_nil(reader_i.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
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

  return 0;
}
