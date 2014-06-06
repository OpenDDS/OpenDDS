/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tests/Utils/DDSApp.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "tests/Utils/ListenerRecorder.h"
#include <dds/DCPS/Service_Participant.h>
#include <model/Sync.h>
#include <stdexcept>
#include <ctime>

#include "dds/DCPS/StaticIncludes.h"

using namespace TestUtils;

namespace {
  typedef std::vector< ::Xyz::Foo> Messages;
  bool valid(const Messages& msgs, int& expected_count)
  {
    if (msgs.empty()) {
      std::cout << "ERROR: no messages received\n";
      return false;
    }
    bool valid = true;
    const size_t size = msgs.size();
    expected_count = -1;
    long expected_seq = 0;
    for(size_t index = 0; index < size; ++index, ++expected_seq) {
      const ::Xyz::Foo& msg = msgs[index];
      if (expected_count == -1) {
        expected_count = msg.key;
      }
      else if (msg.key != expected_count) {
        std::cout << "ERROR: key of " << msg.key
                  << " for message # " << index
                  << " does not match expected count of "
                  << expected_count << "\n";
        valid = false;
      }

      if (expected_seq != msg.c) {
        std::cout << "ERROR: for message # " << index
                  << " expected " << expected_seq
                  << " but received " << (int)msg.c << "\n";
        valid = false;
      }
    }
    return valid;
  }
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  std::cout << "Sub Creating App\n";
  DDSApp dds(argc, argv);
  int status = 0;
  try {
    std::cout << "Sub Creating topic\n";
    DDSFacade< ::Xyz::FooDataWriterImpl> topic = dds.topic< ::Xyz::FooDataWriterImpl>("bar");
    // Create Listener
    ListenerRecorder< ::Xyz::Foo, ::Xyz::FooDataReader>* listener_impl =
      new ListenerRecorder< ::Xyz::Foo, ::Xyz::FooDataReader>;
    DDS::DataReaderListener_var listener(listener_impl);
    // Create data reader for the topic
    std::cout << "Sub Creating reader\n";
    DDS::DataReader_var dr = topic.reader(listener);

    {
      std::cout << "Sub waiting for writer to come and go" << std::endl;
      OpenDDS::Model::ReaderSync rs(dr);
    }
    std::cout << "Sub done waiting\n";

    const Messages msgs = listener_impl->messages();

    int expected_count = -1;
    if (!valid(msgs, expected_count)) {
      status = -1;
    }

    if (msgs.size() == expected_count) {
      std::cout << "Got all " << msgs.size()
                << " samples" << std::endl;
    } else {
      std::cout << "ERROR: Got " << msgs.size()
                << " samples, expected " << expected_count
                << std::endl;
      status = -1;
    }

    // Listener will be cleaned up when reader goes out of scope
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

  return status;
}
