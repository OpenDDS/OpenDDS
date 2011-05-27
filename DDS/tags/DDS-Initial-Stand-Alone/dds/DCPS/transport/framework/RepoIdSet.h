// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_REPOIDSET_H
#define TAO_DCPS_REPOIDSET_H

#include  "dds/DCPS/RcObject_T.h"
#include  "dds/DdsDcpsInfoUtilsC.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"


namespace TAO
{

  namespace DCPS
  {

    class RepoIdSet : public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        typedef ACE_Hash_Map_Manager_Ex<RepoId,
                                        RepoId,
                                        ACE_Hash<RepoId>,
                                        ACE_Equal_To<RepoId>,
                                        ACE_Null_Mutex>        MapType;

        RepoIdSet();
        virtual ~RepoIdSet();

        int insert_id(RepoId id);
        int remove_id(RepoId id);

        ssize_t size() const;

        /// Give access to the underlying map for iteration purposes.
        MapType& map();
        const MapType& map() const;


      private:

        MapType  map_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "RepoIdSet.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DCPS_REPOIDSET_H */
