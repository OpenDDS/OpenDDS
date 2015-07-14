/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tests/Utils/DDSApp.h"
#include "tests/Utils/Options.h"
#include "model/Sync.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "tests/Utils/ListenerRecorder.h"
#include <dds/DCPS/Service_Participant.h>
#include <sstream>
#include <stdexcept>
#include <ctime>

#include "dds/DCPS/StaticIncludes.h"

namespace {
  typedef std::vector< ::Xyz::Foo> Messages;
  bool valid(const Messages& msgs, const long stage, int& expected_count)
  {
    if (msgs.empty()) {
      std::cerr << "ERROR: no messages received\n";
      return false;
    }
    bool valid = true;
    const size_t size = msgs.size();
    expected_count = -1;
    std::set<long> group1;
    std::set<long> group2;
    std::set<long> group3;
    for(size_t index = 0; index < size; ++index) {
      const ::Xyz::Foo& msg = msgs[index];
      if (expected_count == -1) {
        expected_count = msg.key;
      }
      else if (msg.key != expected_count) {
        std::cerr << "ERROR: key of " << msg.key
                  << " for message # " << index
                  << " does not match expected count of "
                  << expected_count << "\n";
        valid = false;
      }

      if (msg.x == 1.0) {
        if (!group1.insert((long)msg.c).second) {
          std::cerr << "ERROR: message # " << index
                    << " group1 repeated value for "
                    << (long)msg.c << "\n";
          valid = false;
        }
      }
      else if (msg.x == 2.0) {
        if (!group2.insert((long)msg.c).second) {
          std::cerr << "ERROR: message # " << index
                    << " group2 repeated value for "
                    << (long)msg.c << "\n";
          valid = false;
        }
      }
      else if (msg.x == 3.0) {
        if (!group3.insert((long)msg.c).second) {
          std::cerr << "ERROR: message # " << index
                    << " group3 repeated value for "
                    << (long)msg.c << "\n";
          valid = false;
        }
      }
      else {
        std::cerr << "ERROR: for message # " << index
                  << " expecting to receive message with x=1.0,"
                  << " 2.0, or 3.0, but received " << msg.x << "\n";
        valid = false;
      }
    }

    if (stage == 2) {
      if ((int)group1.size() == expected_count) {
        std::cerr << "ERROR: Stage 2 received all messages sent in Stage1.\n";
        valid = false;
      }
    }
    else if ((int)group1.size() * 4 < expected_count) {
      const double pct = 100.0 * (double)group1.size()/(double)expected_count;
      std::cerr << "ERROR: received only " << group1.size()
                << " of the " << expected_count
                << " (" << pct
                << "%) expected messages sent in Stage1.\n";
      valid = false;
    }

    if ((int)group2.size() * 4 < expected_count) {
      const double pct = 100.0 * (double)group2.size()/(double)expected_count;
      std::cerr << "ERROR: received only " << group2.size()
                << " of the " << expected_count
                << " (" << pct
                << "%) expected messages sent in Stage2 by the first publisher.\n";
      valid = false;
    }

    if ((int)group3.size() * 4 < expected_count) {
      const double pct = 100.0 * (double)group3.size()/(double)expected_count;
      std::cerr << "ERROR: received only " << group3.size()
                << " of the " << expected_count
                << " (" << pct
                << "%) expected messages sent in Stage2 by the second publisher.\n";
      valid = false;
    }
    return valid;
  }
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  std::stringstream ss;
  ss << "(" << ACE_OS::getpid() << ")";
  const std::string pid = ss.str();
  std::cerr << pid << "Sub Creating App\n";
  int status = 0;
  try {
    ::TestUtils::DDSApp ddsApp(argc, argv);
    std::cerr << pid << "Sub Creating topic\n";
    ::TestUtils::DDSTopicFacade< ::Xyz::Foo > topic =
        ddsApp.topic_facade< ::Xyz::Foo >("bar");

    // need to process after calling topic to ensure all DDS/TAO/ACE command line
    // parameters are already removed
    ::TestUtils::Arguments args;
    args.add_long("stage", 0);
    args.add_bool("verbose", false);
    ::TestUtils::Options options(argc, argv);

    const long stage = options.get<long>("stage");
    if (stage != 1 && stage != 2) {
      std::cerr << "ERROR: Sub command line parameter \"stage\" set to "
                << stage << " should be set to 1 or 2 ";
      return -1;
    }

    // Create Listener
    ::TestUtils::ListenerRecorder< ::Xyz::Foo, ::Xyz::FooDataReader>* listener_impl =
      new ::TestUtils::ListenerRecorder< ::Xyz::Foo, ::Xyz::FooDataReader>;
    listener_impl->verbose(options.get<bool>("verbose"));
    DDS::DataReaderListener_var listener(listener_impl);
    // Create data reader for the topic
    std::cerr << pid << "Sub Creating Stage " << stage << " reader\n";
    DDS::DataReader_var dr = topic.reader(listener);


    {
      std::cerr << pid << "Sub Stage " << stage
                << " waiting for 2 writer to come and go" << std::endl;
      OpenDDS::Model::ReaderSync rs(dr, 2);
    }
    std::cerr << pid << "Sub Stage " << stage << " done waiting\n";

    const Messages msgs = listener_impl->messages();

    int expected_count = -1;
    if (!valid(msgs, stage, expected_count)) {
      status = -1;
    }

    std::cerr << pid << "Sub DDSApp going out of scope\n";
    // Listener will be cleaned up when reader goes out of scope
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = -1;
  } catch (const std::exception& ex) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
               ex.what()));
    status = -1;
  } catch (const std::string& msg) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
               msg.c_str()));
    status = -1;
  }
  std::cerr << pid << "Sub returning status=" << status << "\n";

  return status;
}
