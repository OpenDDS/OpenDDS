/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/InfoRepoDiscovery/InfoC.h"
#include "dds/DCPS/PoolAllocator.h"

#include "ace/Arg_Shifter.h"
#include "ace/Argv_Type_Converter.h"

#include <iostream>



const ACE_TCHAR* ior = ACE_TEXT("file://dcps_ir.ior");



void print_usage () {
  std::cout << std::endl
    << "USAGE: dcpsinfo_dump [-k ior] [-h]" << std::endl
    << "       -k <ior>  use provided DCPSInfoRepo ior. default: "
    << ior << std::endl
    << "       -h        print help and exit" << std::endl;
}

int parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left())
  {
    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-k"))) != 0) {
      ior = currentArg;
      arg_shifter.consume_arg();

    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-h")) == 0) {
      print_usage();
      return 1;
    } else {
      arg_shifter.ignore_arg();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  if (parse_args(argc, argv) != 0)
    return 1;

  try
    {
      ACE_Argv_Type_Converter converter(argc, argv);

      CORBA::ORB_var orb =
        CORBA::ORB_init(converter.get_argc(), converter.get_ASCII_argv(), "");

      CORBA::Object_var tmp =
          orb->string_to_object (ACE_TEXT_ALWAYS_CHAR(ior));
      OpenDDS::DCPS::DCPSInfo_var info =
          OpenDDS::DCPS::DCPSInfo::_narrow (tmp.in ());

      if (CORBA::is_nil(info.in()) )
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                             "(%P|%t) ERROR: Nil OpenDDS::DCPS::DCPSInfo reference <%s>\n",
                             ior),
                            1);
        }

      OPENDDS_STRING state = info->dump_to_string();

      std::cout << state.c_str() << std::endl;

      // clean up the orb
      orb->destroy ();
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in publisher.cpp:");
      return 1;
    }

  return 0;
}
