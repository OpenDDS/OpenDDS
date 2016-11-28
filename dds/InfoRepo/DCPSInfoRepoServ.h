/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPSINFOREPOSERV_H
#define DCPSINFOREPOSERV_H

#include <orbsvcs/Shutdown_Utilities.h>

#include <string>

#include "tao/PortableServer/Servant_var.h"

#include "DCPSInfoRepoServ_Export.h"
#include "FederatorConfig.h"
#include "FederatorManagerImpl.h"
#include "ShutdownInterface.h"

#include "ace/Event_Handler.h"
#include "ace/Condition_Thread_Mutex.h"

class OpenDDS_DCPSInfoRepoServ_Export InfoRepo
  : public ShutdownInterface, public ACE_Event_Handler {
public:
  struct InitError {
    InitError(const char* msg)
        : msg_(msg) {};
    std::string msg_;
  };

  InfoRepo(int argc, ACE_TCHAR *argv[]);
  ~InfoRepo();
  void run();

  /// ShutdownInterface used to schedule a shutdown.
  virtual void shutdown();

  /// shutdown() and wait for it to complete: cannot be called from the reactor
  /// thread.
  void sync_shutdown();

  /// Handler for the reactor to dispatch finalization activity to.
  virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE);

private:
  void init();
  void usage(const ACE_TCHAR * cmd);
  void parse_args(int argc, ACE_TCHAR *argv[]);

  /// Actual finalization of service resources.
  void finalize();

  CORBA::ORB_var orb_;

  ACE_TString ior_file_;
  std::string listen_address_str_;
  int listen_address_given_;
  bool use_bits_;
  bool resurrect_;
  ACE_Time_Value reassociate_delay_;

  /// Flag to indicate that finalization has already occurred.
  bool finalized_;
  bool servant_finalized_;

  /// Repository Federation behaviors
  OpenDDS::Federator::ManagerImpl federator_;
  OpenDDS::Federator::Config      federatorConfig_;

  PortableServer::Servant_var<TAO_DDS_DCPSInfo_i> info_servant_;

  ACE_Thread_Mutex lock_;
  ACE_Condition_Thread_Mutex cond_;
  bool shutdown_complete_;

  ACE_Time_Value dispatch_cleanup_delay_;
};

class OpenDDS_DCPSInfoRepoServ_Export InfoRepo_Shutdown :
      public Shutdown_Functor {
public:
  InfoRepo_Shutdown(InfoRepo& ir);

  void operator()(int which_signal);
private:
  InfoRepo& ir_;
};

#endif  /* DCPSINFOREPOSERV_H */
