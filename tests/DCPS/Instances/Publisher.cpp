/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tests/DCPS/Instances/KeyedDataTypeSupportImpl.h"
#include "tests/DCPS/Instances/NoKeyDataTypeSupportImpl.h"
#include "TestException.h"
#include "tests/Utils/DDSApp.h"
#include "tests/Utils/Options.h"
#include <dds/DCPS/Service_Participant.h>
#include "PubDriver.h"
#include <sstream>
#include <stdexcept>

#include "dds/DCPS/StaticIncludes.h"


int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  try
  {
    ::TestUtils::DDSApp ddsApp(argc, argv);

    // Parse App options
    ::TestUtils::Arguments args(true);
    args.add_bool("keyed_data",true);
    args.add_long("num_threads_to_write",1);
    args.add_bool("multiple_instances",false);
    args.add_long("num_writes_per_thread",1);
    args.add_long("max_samples_per_instance",::DDS::LENGTH_UNLIMITED);
    args.add_long("history_depth",1);
    args.add_long("write_delay_msec",0);
    args.add_long("data_dropped",0);
    args.add_long("num_writers",1);

    ::TestUtils::Options options(argc, argv, args);

    if ( options.get<bool>("keyed_data") == true ) {
      PubDriver< ::Xyz::KeyedData > driver;
      driver.run(ddsApp, options);
      ddsApp.cleanup();
    } else {
      PubDriver< ::Xyz::NoKeyData > driver;
      driver.run(ddsApp, options);
      ddsApp.cleanup();
    }

  }
  catch (const TestException&)
  {

    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) PubDriver TestException.\n")));

  } catch (const CORBA::Exception& e) {

    e._tao_print_exception("Exception caught in main():");
    return -1;

  } catch (std::exception& ex) {

    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: main() - %C\n"), ex.what()),
                      -1);

  } catch (std::string& msg) {

    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: main() - %C\n"), msg.c_str()),
                      -1);
  }

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Pub DDSApp going out of scope (shutdown)\n")));

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Pub returning status=0\n")));

  return 0;
}
