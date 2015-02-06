
#include <model/Sync.h>
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/ExchangeTraits.h"
#include <model/Sync.h>

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  TradeDataWriter_var trd_message_writer;
  QuoteDataWriter_var qt_message_writer;
  Trade trd_message;
  Quote qt_message;
  try {
    OpenDDS::Model::Application application(argc, argv);
    ExchangeLib::DefaultExchangeType model(application, argc, argv);

    using OpenDDS::Model::ExchangeLib::Elements;

    DDS::DataWriter_var trd_writer = model.writer( Elements::DataWriters::TradesWriter);
    DDS::DataWriter_var qt_writer = model.writer( Elements::DataWriters::QuotesWriter);

    // START OF EXISTING MESSENGER EXAMPLE CODE
    trd_message_writer = TradeDataWriter::_narrow(trd_writer.in());

    if (CORBA::is_nil(trd_message_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }
    qt_message_writer = QuoteDataWriter::_narrow(qt_writer.in());

    if (CORBA::is_nil(qt_message_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }

    {
      OpenDDS::Model::WriterSync td_ws(trd_writer);
      {
        OpenDDS::Model::WriterSync qt_ws(qt_writer);

        // Write samples
        trd_message.symbol = CORBA::string_dup("MSFT");
        trd_message.price = 26.8;
        trd_message.size = 200;
        qt_message.symbol = CORBA::string_dup("MSFT");
        qt_message.bid = 26.79;
        qt_message.ask = 26.81;

        for (int i = 0; i < 10; i++) {
          std::cout << "writing" << std::endl;
          DDS::ReturnCode_t td_error = trd_message_writer->write(trd_message, DDS::HANDLE_NIL);

          if (td_error != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                       ACE_TEXT(" write returned %d!\n"), td_error));
          }
          DDS::ReturnCode_t qt_error = qt_message_writer->write(qt_message, DDS::HANDLE_NIL);
          if (qt_error != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                       ACE_TEXT(" write returned %d!\n"), qt_error));
          }
        }
      }
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
  std::cout << "publisher disposing" << std::endl;
  trd_message_writer->dispose(trd_message, DDS::HANDLE_NIL);
  qt_message_writer->dispose(qt_message, DDS::HANDLE_NIL);
  return 0;
}

