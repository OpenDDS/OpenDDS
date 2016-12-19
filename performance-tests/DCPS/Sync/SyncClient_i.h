#ifndef _SYNC_CLIENT_I_
#define _SYNC_CLIENT_I_

#include "SyncS.h"
#include "Sync_export.h"

#include "ace/Task.h"
#include "ace/Synch.h"

#include <string>

/****
 * This class registers itself on an independent
 * ORB. This ORB can either be passed in (via the
 * constructor) or (in case of being passed a null
 * ORB_Ptr) will be created by the SyncClient itself
 ***/

class Sync_Export SyncClient_i : public POA_Sync::Client, public ACE_Task_Base
{
 public:
  typedef std::string InitError;
  enum Role {Pub, Sub};

  SyncClient_i (const std::string& sync_server, CORBA::ORB_ptr orb
                , Role role)
    throw (InitError);
  virtual ~SyncClient_i (void);

  void way_point_reached (int way_point);

  // synchrously wait for server notification.
  // If a notification is found in the pipe,
  // this will return immediately.
  void get_notification (void);

 protected:
  Sync::Server_ptr server_ref (void) const;

 private:

  virtual int svc (void);

  // notification from SyncServer
  virtual void proceed (void);

  // cleans up the notification pipeline.
  // Invoke this before notifying the SyncServer.
  void clean (void);

 private:
  // object is in shutdown mode.
  bool shutdown_;

  // Notification pipeline
  bool notification_;

  Sync::Client_var sync_client_;
  Sync::Server_var sync_server_;

  CORBA::ORB_var orb_;
  PortableServer::POA_var root_poa_;
  PortableServer::POAManager_var poa_manager_;

  Sync::Id my_id_;

  ACE_SYNCH_MUTEX lock_;
};

#endif //  _SYNC_CLIENT_I_
