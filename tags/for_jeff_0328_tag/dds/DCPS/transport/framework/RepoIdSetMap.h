// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_REPOIDSETMAP_H
#define TAO_DCPS_REPOIDSETMAP_H

#include  "dds/DCPS/dcps_export.h"
#include  "RepoIdSet.h"
#include  "RepoIdSet_rch.h"
#include  "dds/DCPS/Definitions.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"

namespace TAO
{

  namespace DCPS
  {

    class TAO_DdsDcps_Export RepoIdSetMap
    {
      public:

        RepoIdSetMap();
        virtual ~RepoIdSetMap();

        int        insert(RepoId key, RepoId value);
        RepoIdSet* find(RepoId key);

        int        remove(RepoId key, RepoId value);
        RepoIdSet* remove_set(RepoId key);

        int release_publisher(RepoId subscriber_id, RepoId publisher_id);

        ssize_t size() const;


      private:

        RepoIdSet* find_or_create(RepoId key);


        typedef ACE_Hash_Map_Manager_Ex<RepoId,
                                        RepoIdSet_rch,
                                        ACE_Hash<RepoId>,
                                        ACE_Equal_To<RepoId>,
                                        ACE_Null_Mutex>        MapType;

        MapType  map_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "RepoIdSetMap.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DCPS_REPOIDSETMAP_H */
