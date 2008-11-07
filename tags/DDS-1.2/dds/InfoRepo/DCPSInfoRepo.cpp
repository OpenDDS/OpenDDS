/*
 * $Id$
 */

#include "DcpsInfo_pch.h"

#include <iostream>

#include "tao/ORB_Core.h"

#include "DCPSInfoRepoServ.h"

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      InfoRepo infoRepo (argc, argv);

      InfoRepo_Shutdown ir_shutdown (infoRepo);
      Service_Shutdown service_shutdown(ir_shutdown);

      infoRepo.run ();
    }
  catch (InfoRepo::InitError& ex)
    {
      std::cerr << "Unexpected initialization Error: "
                << ex.msg_ << std::endl;
      return -1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
        "ERROR: ::DDS DCPS Info Repo caught exception");

      return -1;
    }

  return 0;
}
