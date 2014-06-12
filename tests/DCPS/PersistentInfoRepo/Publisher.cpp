/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tests/Utils/DDSApp.h"
#include <dds/DCPS/Service_Participant.h>
#include <model/Sync.h>
#include <stdexcept>

#include "dds/DCPS/StaticIncludes.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

using namespace TestUtils;
int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  std::cout << "Pub Creating App\n";
  DDSApp dds(argc, argv);
  try {
    std::cout << "Pub Creating topic\n";
    DDSFacade< ::Xyz::FooDataWriterImpl> topic = dds.facade< ::Xyz::FooDataWriterImpl>("bar");
    // Create data writer for the topic
    // ?? fix to code gen will allow using ::Xyz::Foo
    std::cout << "Pub Creating writer\n";
    ::Xyz::FooDataWriter_var msg_writer = topic.writer();

    {
      std::cout << "Pub waiting for reader\n";
      // Block until Subscriber is available
      OpenDDS::Model::WriterSync ws(DDSApp::datawriter(msg_writer), 1);
      std::cout << "Pub done waiting for reader\n";

      // Initialize samples
      ::Xyz::Foo message;

      // Override message count
      long msg_count = 100;

      char number[20];

      std::cout << "Pub sending\n";
      for (int i = 0; i<msg_count; ++i) {
        // Prepare next sample
        sprintf(number, "foo %d", i);
        message.key = msg_count;
        message.c = (char)i;
        message.x = 1.0;
        message.y = 1.0;

        // Publish the message
        DDS::ReturnCode_t error = DDS::RETCODE_TIMEOUT;
        while (error == DDS::RETCODE_TIMEOUT) {
          error = msg_writer->write(message, DDS::HANDLE_NIL);
          if (error == DDS::RETCODE_TIMEOUT) {
            ACE_ERROR((LM_ERROR, "Timeout, resending %d\n", i));
          } else if (error != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("ERROR: %N:%l: main() -")
                       ACE_TEXT(" write returned %d!\n"), error));
          }
        }
      }

      std::cout << "Pub waiting for acks from sub" << std::endl;
    }
    std::cout << "Pub done waiting for acks from sub" << std::endl;
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  } catch (std::runtime_error& err) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                      err.what()), -1);
  } catch (std::string& msg) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                      msg.c_str()), -1);
  }

  return 0;
}
