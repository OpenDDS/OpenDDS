
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include "model/MultiTopicTraits.h"

const char* symbols[] = {"AAPL", "GOOG", "MSFT", "XLNX"};
const char* names[] = {"Apple", "Google", "Microsoft", "Xilinx"};
const double prices[] = {345.38, 609.86, 27.70, 33.51};
int sym_count = sizeof(symbols) / sizeof(const char*);

int send_reference_data(MultiTopicLib::DefaultMultiTopicType& model)
{
    using OpenDDS::Model::MultiTopicLib::Elements;
    DDS::DataWriter_var writer = model.writer(Elements::DataWriters::ref_data_writer);
    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::ReferenceDataDataWriter_var rd_writer =
      data1::ReferenceDataDataWriter::_narrow(writer.in());

    if (CORBA::is_nil(rd_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }

    // Block until Subscriber is available
    DDS::StatusCondition_var condition = writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };
    DDS::Duration_t timeout = { 30, 0 };

    do {
      std::cout << "pub waiting for ref data match" << std::endl;
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wait failed!\n")),
                         -1);
      }

      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_publication_matched_status failed!\n")),
                         -1);
      }

    } while (matches.current_count < 1);

    ws->detach_condition(condition);

    std::cout << "pub writing ref data" << std::endl;
    // Write samples
    data1::ReferenceData rd;

    for (int i = 0; i < sym_count; i++) {
      rd.symbol       = CORBA::string_dup(symbols[i]);
      rd.name         = CORBA::string_dup(names[i]);

      DDS::ReturnCode_t error = rd_writer->write(rd, DDS::HANDLE_NIL);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                   ACE_TEXT(" write returned %d!\n"), error));
      }
    }

    // Wait for samples to be acknowledged
    std::cout << "pub waiting for ref data acks" << std::endl;
    if (rd_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                        ACE_TEXT(" wait_for_acknowledgments failed!\n")),
                       -1);
    }
    return 0;
}

int send_trade_data(MultiTopicLib::DefaultMultiTopicType& model)
{
    using OpenDDS::Model::MultiTopicLib::Elements;
    DDS::DataWriter_var writer = model.writer(Elements::DataWriters::trade_writer);
    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::TradeDataWriter_var trade_writer =
      data1::TradeDataWriter::_narrow(writer.in());

    if (CORBA::is_nil(trade_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }

    // Block until Subscriber is available
    DDS::StatusCondition_var condition = writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };
    DDS::Duration_t timeout = { 30, 0 };

    do {
      std::cout << "pub waiting for trade data match" << std::endl;
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wait failed!\n")),
                         -1);
      }

      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_publication_matched_status failed!\n")),
                         -1);
      }

    } while (matches.current_count < 1);

    ws->detach_condition(condition);

    std::cout << "pub writing trades" << std::endl;
    // Write samples
    data1::Trade trade;

    trade.quantity     = 200;
    trade.seq          = 0;

    for (int sym_index = 0; sym_index < sym_count; sym_index++) {
      trade.symbol       = CORBA::string_dup(symbols[sym_index]);
      trade.last         = prices[sym_index];
      for (int i = 0; i < 10; i++) {
        DDS::ReturnCode_t error = trade_writer->write(trade, DDS::HANDLE_NIL);
        ++trade.seq;
        trade.last += 0.01;

        if (error != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                     ACE_TEXT(" write returned %d!\n"), error));
        }
      }

      // Wait for samples to be acknowledged
      std::cout << "pub waiting for trade data acks for " << trade.symbol << std::endl;
      if (trade_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wait_for_acknowledgments failed!\n")),
                         -1);
      }
  }
  return 0;
}

int ACE_TMAIN(int argc, char** argv)
{
  int rc = 0;
  try {
    OpenDDS::Model::Application application(argc, argv);
    MultiTopicLib::DefaultMultiTopicType model(application, argc, argv);


    rc = send_reference_data(model);
    std::cout << "send_reference_data returned " << rc << std::endl;
    rc = rc || send_trade_data(model);
    std::cout << "send_trade_data returned " << rc << std::endl;

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

  std::cout << "pub exiting" << std::endl;
  return rc;
}

