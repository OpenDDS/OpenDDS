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
#include "SubDriver.h"
#include <sstream>
#include <stdexcept>

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  try
  {
    // Initalize ParticapantFactory and parse ORB options
    ::TestUtils::DDSApp dds(argc, argv);

    // Parse App options
    ::TestUtils::Arguments args(true);
    args.add_long("foo_type",1);
    args.add_long("num_writes",1);
    args.add_long("receive_delay_msec",0);
    args.add_bool("verbose",true);

    ::TestUtils::Options options(argc, argv, args);

    const long FOO1_TYPE = 1;
    const long FOO2_TYPE = 2;

    switch (options.get<long>("foo_type"))
    {

      case FOO2_TYPE:
        {
          SubDriver< ::Xyz::Foo2TypeSupportImpl> driver;
          driver.run(dds, options);
        }
      break;

      case FOO1_TYPE:
      default:
        {
          SubDriver< ::Xyz::Foo1TypeSupportImpl> driver;
          driver.run(dds, options);
        }
      break;
    }

  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) SubDriver TestException.\n"));

  } catch (const CORBA::Exception& e) {

    e._tao_print_exception("Exception caught in main():");
    return -1;

  } catch (std::exception& ex) {

    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"), ex.what()), -1);

  } catch (std::string& msg) {

    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"), msg.c_str()), -1);

  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) Sub DDSApp going out of scope (shutdown)\n"));
  ACE_DEBUG((LM_DEBUG, "(%P|%t) Sub returning status=0\n"));
  return 0;
}
