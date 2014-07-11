/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPSInfoRepoServ.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"
#endif

#include "dds/DCPS/async_debug.h"

#include "tao/ORB_Core.h"

#include <iostream>

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {

     //### Debug statements to track where connection is failing
     if (OpenDDS::DCPS::ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:InfoRepoDiscovery::ACE_TMAIN --> begin\n"));
     if (OpenDDS::DCPS::ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:InfoRepoDiscovery::ACE_TMAIN --> START to instantiate infoRepo\n"));
    InfoRepo infoRepo(argc, argv);

    //### Debug statements to track where connection is failing
    if (OpenDDS::DCPS::ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:InfoRepoDiscovery::ACE_TMAIN --> DONE instantiating infoRepo\n"));

    InfoRepo_Shutdown ir_shutdown(infoRepo);
    Service_Shutdown service_shutdown(ir_shutdown);

    //### Debug statements to track where connection is failing
    if (OpenDDS::DCPS::ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:InfoRepoDiscovery::ACE_TMAIN --> about to 'run' infoRepo\n"));

    infoRepo.run();

    //### Debug statements to track where connection is failing
    if (OpenDDS::DCPS::ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:InfoRepoDiscovery::ACE_TMAIN --> post-'run' infoRepo\n"));

  } catch (InfoRepo::InitError& ex) {
    std::cerr << "Unexpected initialization Error: "
              << ex.msg_ << std::endl;
    return -1;

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: DDS DCPS Info Repo caught exception");
    return -1;
  }
  //### Debug statements to track where connection is failing
  if (OpenDDS::DCPS::ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:InfoRepoDiscovery::ACE_TMAIN --> end\n"));

  return 0;
}
