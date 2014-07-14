/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tests/DCPS/Instances/InstanceFoo1TypeSupportImpl.h"
#include "tests/DCPS/Instances/InstanceFoo2TypeSupportImpl.h"
#include "TestException.h"
#include "tests/Utils/DDSApp.h"
#include "tests/Utils/Options.h"
#include "PubDriver.h"
#include <sstream>
#include <stdexcept>


int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  try
  {
    ::TestUtils::DDSApp dds(argc, argv);

    // Parse App options
    ::TestUtils::Arguments args(true);
    args.add_long("foo_type",1);
    args.add_long("num_threads_to_write",1);
    args.add_bool("multiple_instances",false);
    args.add_long("num_writes_per_thread",1);
    args.add_long("max_samples_per_instance",::DDS::LENGTH_UNLIMITED);
    args.add_long("history_depth",1);
    args.add_long("has_key_flag",1);
    args.add_long("write_delay_msec",0);
    args.add_long("data_dropped",0);
    args.add_long("num_writers",1);

    ::TestUtils::Options options(argc, argv, args);

    const long FOO1_TYPE = 1;
    const long FOO2_TYPE = 2;


    switch (options.get<long>("foo_type"))
    {

      case FOO2_TYPE:
        {
          PubDriver< ::Xyz::Foo2TypeSupportImpl> driver;
          driver.run(dds, options);
        }
      break;

      case FOO1_TYPE:
      default:
        {
          PubDriver< ::Xyz::Foo1TypeSupportImpl> driver;
          driver.run(dds, options);
        }
      break;

    }

  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) PubDriver TestException.\n"));

  } catch (const CORBA::Exception& e) {

    e._tao_print_exception("Exception caught in main():");
    return -1;

  } catch (std::exception& ex) {

    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"), ex.what()), -1);

  } catch (std::string& msg) {

    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"), msg.c_str()), -1);

  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) Pub DDSApp going out of scope (shutdown)\n"));
  ACE_DEBUG((LM_DEBUG, "(%P|%t) Pub returning status=0\n"));

  return 0;
}
