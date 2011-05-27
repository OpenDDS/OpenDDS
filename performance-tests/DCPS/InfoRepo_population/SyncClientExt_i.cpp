#include "SyncClientExt_i.h"

#include "SyncExtC.h"

#include <iostream>

SyncClientExt_i::SyncClientExt_i (const std::string& sync_server
                                  , CORBA::ORB_ptr orb, Role role)
  throw (SyncClient_i::InitError)
  : SyncClient_i (sync_server, orb, role)
{ }

SyncClientExt_i::~SyncClientExt_i (void)
{ }

void
SyncClientExt_i::publish (SyncExt::Role role, int instances
                          , int msecs)
{
  try
    {
      SyncExt::Collector_var collector
        = SyncExt::Collector::_narrow (this->server_ref());
      if (CORBA::is_nil (collector.in()))
        {
          std::cerr << "SyncClientExt_i::push> Unable to narrow Collector."
                    << std::endl;
          return;
        }

      collector->publish (role, instances, msecs);
    }
  catch (CORBA::Exception&)
    {
      std::cerr << "SyncClientExt_i::push> Caught a CORBA exception."
                << std::endl;
       return;
    }
}
