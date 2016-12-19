#ifndef _SYNC_SERVER_I_
#define _SYNC_SERVER_I_

#include "SyncS.h"
#include "Sync_export.h"

#include "tao/IORTable/IORTable.h"
#include "ace/Task.h"

#include <map>
#include <string>

class Sync_Export SyncServer_i : public virtual POA_Sync::Server, public ACE_Task_Base
{
 public:
  typedef std::string InitError;

  SyncServer_i (size_t pub_count, size_t sub_count
                , CORBA::ORB_ptr orb, bool write_ior=true) throw (InitError, CORBA::ORB::InvalidName);
  virtual ~SyncServer_i (void);

  // synchrously wait for session completion.
  bool wait_session (void);

 private:

  virtual int svc (void);

  virtual void register_me (::Sync::Role rol, ::Sync::Client_ptr callback,
                            ::Sync::Id_out ide);

  virtual void unregister (::Sync::Id ide);

  virtual void way_point_reached (::Sync::Id ide,
                                  ::Sync::WayPoint way_point);
 private:
  typedef struct Cl
  {
    ::Sync::Id id;
    ::Sync::Client_var callback;
    ::Sync::Role role;
    ::Sync::WayPoint way_point;

    Cl (::Sync::Id i, ::Sync::Client_var c
        , ::Sync::Role r, ::Sync::WayPoint w)
      : id (i), callback (c)
         , role (r), way_point (w)
    { };
  } ClientInfo;

  typedef std::map < ::Sync::Id, ClientInfo> ClientInfos;

  ClientInfos pubs_;
  ClientInfos subs_;

  size_t count_;

  const size_t pub_count_;
  const size_t sub_count_;

  CORBA::ORB_var orb_;
  PortableServer::POA_var root_poa_;
  PortableServer::POAManager_var poa_manager_;
  IORTable::Table_var table_;

 private:

  bool synched (void);
  void notify (void);
  void reset (void);

  bool shutdown_;
  std::string ior_file_;
};

#endif //  _SYNC_SERVER_I_
