// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "DataLinkCleanupTask.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "TransportImpl.h"

OpenDDS::DCPS::DataLinkCleanupTask::DataLinkCleanupTask (TransportImpl* transportImpl)
  : transportImpl_ (transportImpl)
{
  DBG_ENTRY_LVL("DataLinkCleanupTask", "DataLinkCleanupTask", 5);
}

OpenDDS::DCPS::DataLinkCleanupTask::~DataLinkCleanupTask ()
{
  DBG_ENTRY_LVL("DataLinkCleanupTask", "~DataLinkCleanupTask", 5);

  this->transportImpl_ = 0;
}

void
OpenDDS::DCPS::DataLinkCleanupTask::execute (DataLink_rch& dl)
{
  DBG_ENTRY_LVL("DataLinkCleanupTask", "execute", 5);

  // Assumes that the DataLink is safe for now.
  // ciju: I don't believe there are any thread issues here. If any
  // the risk seems minimal.
  // Not sure about the above statement. Associations could change while
  // the Id sequence is being created. That could be trouble.

  // pub -> sub

  // The pub_map_ has an entry for each pub_id
  // Create iterator to traverse Publisher map.
  ReceiveListenerSetMap::MapType& pub_map = dl->pub_map_.map();
  for (ReceiveListenerSetMap::MapType::iterator pub_map_iter = pub_map.begin();
       pub_map_iter != pub_map.end(); )
    {
      // Extract the pub id
      RepoId pub_id = pub_map_iter->first;
      // Each pub_id (may)has an associated DataWriter
      // Dependends upon whether we are an actual pub or sub.
      DataWriterImpl *dw = this->transportImpl_->find_publication (pub_id, true);

      ReceiveListenerSet_rch sub_id_set = pub_map_iter->second;
      // The iterator seems to get corrupted if the element currently
      // being pointed at gets unbound. Hence advance it.
      ++pub_map_iter;

      // Check is DataWriter exists (could have been deleted before we got here.
      if (dw != NULL)
        {
          // Each pub-id is mapped to a bunch of sub-id's
          //ReceiveListenerSet_rch sub_id_set = pub_entry->int_id_;
          ReaderIdSeq sub_ids;
          sub_id_set->get_keys (sub_ids);

          // after creating remote id sequence, remove from DataWriter
          // I believe the 'notify_lost' should be set to false, since
          // it doesn't look like we meet any of the conditions for setting
          // it true. Check interface documentations.
          dw->remove_associations (sub_ids, false);

          // Since we requested a safe copy, we now need to remove the local reference.
          dw->_remove_ref ();
        }
    }

  // sub -> pub
  // Create iterator to traverse Subscriber map.
  RepoIdSetMap::MapType& sub_map = dl->sub_map_.map();
   for (RepoIdSetMap::MapType::iterator sub_map_iter = sub_map.begin();
   sub_map_iter != sub_map.end(); )
  {
    // Extract the sub id
    RepoId sub_id = sub_map_iter->first;
    // Each sub_id (may)has an associated DataReader
    // Dependends upon whether we are an actual pub or sub.
    DataReaderImpl *dr = this->transportImpl_->find_subscription (sub_id, true);

    RepoIdSet_rch pub_id_set = sub_map_iter->second;
    // The iterator seems to get corrupted if the element currently
    // being pointed at gets unbound. Hence advance it.
    ++sub_map_iter;

    // Check id DataReader exists (could have been deleted before we got here.)
    if (dr != NULL)
      {
        // Each sub-id is mapped to a bunch of pub-id's
        ssize_t pub_ids_count = pub_id_set->size();
        WriterIdSeq pub_ids (pub_ids_count);
        pub_ids.length (pub_ids_count);

        int count = 0;
        // create a sequence of associated pub-id's
        for (RepoIdSet::MapType::iterator pub_ids_iter = pub_id_set->map().begin();
          pub_ids_iter != pub_id_set->map().end(); ++pub_ids_iter)
            {
              pub_ids [count++] = pub_ids_iter->first;
            }

          // after creating remote id sequence, remove from DataReader
          // I believe the 'notify_lost' should be set to false, since
          // it doesn't look like we meet any of the conditions for setting
          // it true. Check interface documentations.
          dr->remove_associations (pub_ids, false);

          // Since we requested a safe copy, we now need to remove the local reference.
          dr->_remove_ref ();
      }
  }
}
