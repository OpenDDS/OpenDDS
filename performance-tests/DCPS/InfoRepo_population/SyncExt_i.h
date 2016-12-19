#ifndef _DATASYNC_I_
#define _DATASYNC_I_

#include "SyncExtS.h"
#include "SyncServer_i.h"

class SyncExt_i : public virtual POA_SyncExt::Collector,
                   public virtual SyncServer_i
{
 public:

  SyncExt_i (size_t pub_count, size_t sub_count
              , CORBA::ORB_ptr orb);
  virtual ~SyncExt_i (void);

  virtual void publish (SyncExt::Role rol
                        , ::CORBA::Long instances
                        , ::CORBA::Long msecs);

  void print_results (void);

 private:
  typedef struct {
    int instances;
    int msecs;
  } EntityData;

  const int entities_len;
  EntityData entities_data [SyncExt::Subscriber+1];
};

#endif // _DATASYNC_I_
