#include "SyncClient_i.h"

#include "ace/OS_NS_unistd.h"
#include "ace/String_Base.h"

#include <string>

SyncClient_i::SyncClient_i (const std::string& sync_server
                            , CORBA::ORB_ptr orb, Role role)
  throw (SyncClient_i::InitError)
  : shutdown_ (false), notification_ (false)
{
  try
    {
      if (CORBA::is_nil (orb))
        {
          // create new ORB
          int argc = 2;
          char* argv[] = {(char*) "-ORBDebuglevel", (char*) "0"};
          orb_ = CORBA::ORB_init (argc, argv, "SyncClient");
          if (CORBA::is_nil (orb_.in())) {
            throw InitError ("SyncClient_i::ctr> Orb init failed.");
          }
        }
      else {
        orb_ = CORBA::ORB::_duplicate (orb);
      }

      CORBA::Object_var poaObj = orb_->resolve_initial_references ("RootPOA");
      root_poa_ = PortableServer::POA::_narrow (poaObj.in ());

      poa_manager_ = root_poa_->the_POAManager ();
      poa_manager_->activate ();

      PortableServer::ObjectId_var oid = root_poa_->activate_object (this);
      CORBA::Object_var obj = root_poa_->id_to_reference (oid);
      sync_client_ = Sync::Client::_narrow (obj.in());

      // activate task
      if (this->activate (THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED
                          , 1) != 0) {
        throw InitError ("SyncClient_i::ctr> Task activation failed.");
      }

      std::string sync_srvr;
      if (sync_server.empty()) {
        //sync_srvr = "corbaloc:iiop:localhost:12345/SyncServer";
        sync_srvr = "file://sync.ior";
      }

      obj = orb_->string_to_object (sync_srvr.c_str());
      sync_server_ = ::Sync::Server::_narrow (obj.in());

      if (role == Pub) {
        //ACE_DEBUG ((LM_DEBUG, "(%P|%t) SyncClient_i> Registering self.\n"));
        sync_server_->register_me (Sync::Pub, sync_client_.in(), my_id_);
        //ACE_DEBUG ((LM_DEBUG, "(%P|%t) SyncClient_i> Registered self.\n"));
      }
      else if (role == Sub) {
        sync_server_->register_me (Sync::Sub, sync_client_.in(), my_id_);
      }
    }
  catch ( CORBA::Exception& ex) {
    throw InitError (ex._info().c_str());
  }
}

SyncClient_i::~SyncClient_i (void)
{
  shutdown_ = true;

  sync_server_->unregister (my_id_);
  orb_->destroy ();

  this->wait ();
}

void
SyncClient_i::way_point_reached (::Sync::WayPoint way_point)
{
  this->clean ();
  sync_server_->way_point_reached (my_id_, way_point);
}

void
SyncClient_i::get_notification (void)
{
  while (true)
    {
      {
        ACE_GUARD( ACE_SYNCH_MUTEX, guard, this->lock_);
        if (notification_)
          {
            notification_ = false;
            break;
          }
      }

      ACE_Time_Value small_time(0,250000);
      ACE_OS::sleep (small_time);
    }
}

int
SyncClient_i::svc (void)
{
  try
    {
      orb_->run ();
    }
  catch ( CORBA::Exception& ) {
    return -1;
  }
  return 0;
}

void
SyncClient_i::proceed (void)
{
  ACE_GUARD( ACE_SYNCH_MUTEX, guard, this->lock_);
  notification_ = true;
}

void
SyncClient_i::clean (void)
{
  ACE_GUARD( ACE_SYNCH_MUTEX, guard, this->lock_);
  notification_ = false;
}

Sync::Server_ptr
SyncClient_i::server_ref (void) const
{
  return sync_server_.in();
}
