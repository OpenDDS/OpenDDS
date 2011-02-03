
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include "model/MultiTopicTraits.h"
#include <model/NullReaderListener.h>

class ReaderListener : public OpenDDS::Model::NullReaderListener {
  public: ReaderListener(bool& complete_flag) : complete_(complete_flag)
  { }
  virtual void on_data_available(
    DDS::DataReader_ptr reader)
  ACE_THROW_SPEC((CORBA::SystemException));
  private:
    bool& complete_;
};


// START OF EXISTING MESSENGER EXAMPLE LISTENER CODE

void
ReaderListener::on_data_available(DDS::DataReader_ptr reader)
ACE_THROW_SPEC((CORBA::SystemException))
{
  data1::AnnotatedTradeDataReader_var reader_i =
    data1::AnnotatedTradeDataReader::_narrow(reader);

  if (CORBA::is_nil(reader_i.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  data1::AnnotatedTrade trade;
  DDS::SampleInfo info;

  DDS::ReturnCode_t error = reader_i->take_next_sample(trade, info);

  if (error == DDS::RETCODE_OK) {
    std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
    std::cout << "SampleInfo.instance_state = " << info.instance_state << std::endl;

    if (info.valid_data) {
      std::cout << "AnnotatedTrade: " << std::endl
                << "         symbol  = " << trade.symbol   << std::endl
                << "         seq     = " << trade.seq      << std::endl
                << "         last    = " << trade.last     << std::endl
                << "         qty     = " << trade.quantity << std::endl;

      complete_ = (trade.seq == 9);
    }

  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" take_next_sample failed!\n")));
  }
}

// END OF EXISTING MESSENGER EXAMPLE LISTENER CODE

int ACE_TMAIN(int argc, char** argv)
{
  bool complete = false;
  try {
    OpenDDS::Model::Application application(argc, argv);
    MultiTopicLib::DefaultMultiTopicType model(application, argc, argv);

    using OpenDDS::Model::MultiTopicLib::Elements;

    DDS::TopicDescription_var tv(model.topic(Elements::Participants::part2, Elements::Topics::trades));
    DDS::DataReader_var reader = model.reader( Elements::DataReaders::reader);

    DDS::DataReaderListener_var listener(new ReaderListener(complete));
    reader->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::AnnotatedTradeDataReader_var reader_i =
      data1::AnnotatedTradeDataReader::_narrow(reader);

    if (CORBA::is_nil(reader_i.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
    }

    // We do not get subscription matched notifications on the multitopic
    while (!complete) {
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

  return 0;
}
