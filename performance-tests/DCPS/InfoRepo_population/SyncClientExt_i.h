#ifndef _SYCCLIENTEXT_I_
#define _SYCCLIENTEXT_I_

#include "SyncClient_i.h"
#include "SyncExtC.h"

class SyncClientExt_i : public SyncClient_i
{
 public:
  SyncClientExt_i (const std::string& sync_server, CORBA::ORB_ptr orb
                   , Role role)
    throw (SyncClient_i::InitError);
  virtual ~SyncClientExt_i (void);

  void publish (SyncExt::Role role, int instances
                , int msecs);
};

#endif // _SYCCLIENTEXT_I_
