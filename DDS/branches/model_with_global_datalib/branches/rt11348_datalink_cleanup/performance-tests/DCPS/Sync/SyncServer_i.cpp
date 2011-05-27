#include "SyncServer_i.h"

#include "tao/IORTable/IORTable.h"
#include "ace/String_Base.h"
#include "ace/OS_NS_unistd.h"

#include <iostream>
#include <string>

SyncServer_i::SyncServer_i (size_t pub_count, size_t sub_count
                            , CORBA::ORB_ptr orb) throw (InitError)
  : count_ (0), pub_count_(pub_count), sub_count_(sub_count)
    , shutdown_ (false)
{
  try
    {
      if (CORBA::is_nil (orb))
        {
          // create new ORB
          int argc = 2;
          ACE_TCHAR* argv[] = {"-ORBEndpoint", "iiop://localhost:12345"};
          orb_ = CORBA::ORB_init (argc, (ACE_TCHAR **)argv, "SyncServer");
          if (CORBA::is_nil (orb_.in())) {
            throw InitError ("SyncServer_i::ctr> Orb init failed.");
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
      ::Sync::Server_var sync_server_obj = Sync::Server::_narrow (obj);

      CORBA::String_var ior = orb_->object_to_string (sync_server_obj.in ());
      CORBA::Object_var table_obj =
        orb_->resolve_initial_references("IORTable");
      table_ = IORTable::Table::_narrow(table_obj.in());
      table_->bind ("SyncServer", ior.in());

      // activate task
      if (this->activate (THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED
                          , 1) != 0) {
        throw InitError ("SyncServer_i::ctr> Task activation failed.");
      }
    }
  catch (const CORBA::SystemException& ex)
    {
      throw InitError (ex._info().c_str());
    }

  //ACE_DEBUG ((LM_DEBUG, "SyncServer_i> pubs: %d, subs: %d\n"
  //, pub_count_, sub_count_));
}

SyncServer_i::~SyncServer_i (void)
{
  orb_->destroy ();

  this->wait ();
}

void
SyncServer_i::register_me (::Sync::Role role, ::Sync::Client_ptr callback,
                           ::Sync::Id_out id)
  throw (CORBA::SystemException)
{
  //ACE_DEBUG ((LM_DEBUG, "(%P|%t) SyncServer_i::register_me\n"));

  // Hack to offset connection setup cost later.
  callback->_non_existent ();

  Sync::Client_ptr cl = Sync::Client::_duplicate (callback);
  Cl cl_info (count_, cl, role, -1);

  switch (role)
    {
    case Sync::Sub:
      if (subs_.size() >= sub_count_) {
        return;
      }

      if (subs_.insert (subs_.end()
                        , ClientInfos::value_type(++count_, cl_info))
          != subs_.end()){
        id = count_;
      }
      break;
    case Sync::Pub:
      if (pubs_.size() >= pub_count_) {
        return;
      }

      if (pubs_.insert (pubs_.end()
                        , ClientInfos::value_type(++count_, cl_info))
          != pubs_.end()){
        id = count_;
      }
      break;
    }

  //ACE_DEBUG ((LM_DEBUG, "(%P|%t) SyncServer_i::register_me\n"
  //"Pubs: %d, Subs: %d\n", pubs_.size(), subs_.size()));
}

void
SyncServer_i::unregister (::Sync::Id id)
  throw (CORBA::SystemException)
{
  //ACE_DEBUG ((LM_DEBUG, "(%P|%t) SyncServer_i::unregister\n"));
  if (subs_.find (id) != subs_.end()) {
    subs_.erase (id);
  }
  else if (pubs_.find (id) != pubs_.end()) {
    pubs_.erase (id);
  }

  size_t pub_count = pubs_.size ();
  size_t sub_count = subs_.size ();

  if ((pub_count == 0) && (sub_count == 0)) {
    orb_->shutdown ();
  }
}

void
SyncServer_i::way_point_reached (::Sync::Id id,
                                 ::Sync::WayPoint way_point)
  throw (CORBA::SystemException)
{
  //ACE_DEBUG ((LM_DEBUG, "(%P|%t) SyncServer_i::way_point_reached\n"));

  ClientInfos::iterator iter = subs_.find (id);
  if (iter == subs_.end()) {
    iter = pubs_.find (id);
    if (iter == pubs_.end()) {
      return;
    }
  }


  iter->second.way_point = way_point;

  if ((pubs_.size() == pub_count_)
      && (subs_.size() == sub_count_)
      && synched ()) {
    notify ();
  }
}

bool
SyncServer_i::synched (void)
{
  //ACE_DEBUG ((LM_DEBUG, "(%P|%t) SyncServer_i::synched\n"));

  ::Sync::WayPoint waypoint = pubs_.begin()->second.way_point;

  for (ClientInfos::const_iterator iter = pubs_.begin();
       iter != pubs_.end();
       iter++)
    {
      if (waypoint != iter->second.way_point) {
        return false;
      }
    }

  for (ClientInfos::const_iterator iter = subs_.begin();
       iter != subs_.end();
       iter++)
    {
      if (waypoint != iter->second.way_point) {
        return false;
      }
    }

  return true;
}

void
SyncServer_i::notify (void)
{
  for (ClientInfos::const_iterator iter = pubs_.begin();
       iter != pubs_.end();
       iter++) {
    iter->second.callback->proceed ();
  }

  for (ClientInfos::const_iterator iter = subs_.begin();
       iter != subs_.end();
       iter++) {
    iter->second.callback->proceed ();
  }
}

int
SyncServer_i::svc (void)
{
  try
    {
      orb_->run ();
      shutdown_ = true;
    }
  catch ( CORBA::Exception& ) {
    return -1;
  }
  return 0;
}

bool
SyncServer_i::wait_session (void)
{
  while (true)
    {
      if (shutdown_) {
        break;
      }

      ACE_Time_Value small(0,250000);
      ACE_OS::sleep (small);
    }

  return true;
}
