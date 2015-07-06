/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tests/Utils/DDSApp.h"
#include "tests/Utils/Options.h"
#include "model/Sync.h"

#include <dds/DCPS/Service_Participant.h>
#include <sstream>
#include <stdexcept>

#include "dds/DCPS/StaticIncludes.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  std::stringstream ss;
  ss << "(" << ACE_OS::getpid() << ")";
  const std::string pid = ss.str();

  {
    try {

      std::cerr << pid << "Pub Creating App\n";
      ::TestUtils::DDSApp dds(argc, argv);

      std::cerr << pid << "Pub Creating topic\n";

      ::TestUtils::Arguments args;
      args.add_long("stage", 0);
      args.add_long("messages", 60);
      ::TestUtils::Options options(argc, argv);

      long stage = options.get<long>("stage");
      if (stage != 1 && stage != 2) {
        std::cerr << "ERROR: " << pid
                  << "Pub command line parameter \"stage\" set to "
                  << stage << " should be set to 1 or 2 ";
        return -1;
      }
      const long msg_count = options.get<long>("messages");
      if (msg_count <= 0) {
        std::cerr << "ERROR: " << pid
                  << "Pub command line parameter \"messages\" set to "
                  << msg_count << " should be set greater than 0.\n";
        return -1;
      }

      // for stage 1 send one set of data with 1.0 and one with 2.0
      // for stage 2 send one set of data with 3.0
      float id = (stage == 1 ? 1.0f : 3.0f);

      // Create data writer for the topic
      std::cerr << pid << "Pub Stage " << stage << " Creating writer\n";

      // ?? fix to code gen will allow using ::Xyz::Foo
      ::TestUtils::DDSTopicFacade< ::Xyz::Foo > topic_facade = dds.topic_facade< ::Xyz::Foo >("bar");

      ::Xyz::FooDataWriter_var msg_writer;
      msg_writer = topic_facade.writer();

      for ( ; stage < 3; ++stage, ++id) {

        {
          // each stage number will wait for that same number of readers readers
          std::cerr << pid << "Pub Stage " << stage
                    << " waiting for " << stage << " readers\n";
          // Block until Subscriber is available
          OpenDDS::Model::WriterSync ws(
            ::TestUtils::DDSApp::datawriter(msg_writer), 1);
          std::cerr << pid << "Pub Stage " << stage
                    << " done waiting for reader\n";

          // Initialize samples
          ::Xyz::Foo message;

          char number[20];

          std::cerr << pid << "Pub Stage " << stage << " sending\n";
          for (int i = 0; i<msg_count; ++i) {
            // Prepare next sample
            sprintf(number, "foo %d", i);
            message.key = msg_count;
            message.c = (char)i;
            message.x = id;
            message.y = 1.0;

            // Publish the message
            DDS::ReturnCode_t error = DDS::RETCODE_TIMEOUT;
            while (error == DDS::RETCODE_TIMEOUT) {
              ACE_OS::sleep(1);
              error = msg_writer->write(message, DDS::HANDLE_NIL);
              if (error == DDS::RETCODE_TIMEOUT) {
                ACE_ERROR((LM_ERROR, "(%P|%t) Timeout, resending %d\n", i));
              } else if (error != DDS::RETCODE_OK) {
                ACE_ERROR((LM_ERROR,
                           ACE_TEXT("ERROR: %N:%l: main() -")
                           ACE_TEXT(" write returned %d!\n"), error));
              }
            }
          }

          std::cerr << pid << "Pub Stage " << stage
                    << " waiting for acks from sub" << std::endl;
        }
        std::cerr << pid << "Pub Stage " << stage
                  << " done waiting for acks from sub" << std::endl;
      }
      std::cerr << pid << "Pub DDSTopic going out of scope" << std::endl;
    } catch (const CORBA::Exception& e) {
      e._tao_print_exception("Exception caught in main():");
      return -1;
    } catch (const std::exception& ex) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                        ex.what()), -1);
    } catch (const std::string& msg) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                        msg.c_str()), -1);
    }

    std::cerr << pid << "Pub DDSApp going out of scope (shutdown)" << std::endl;
  }
  std::cerr << pid << "Pub returning status=0" << std::endl;
  return 0;
}
