// -*- C++ -*-
//
// $Id$

#include  "TransportSendStrategy.h"
#include  "TransportReceiveStrategy.h"
#include  "EntryExit.h"


ACE_INLINE void
TAO::DCPS::DataLink::send_start()
{
  DBG_ENTRY("DataLink","send_start");

  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.
  this->send_strategy_->send_start();
}


//MJM: Here is where inlining will become critical.  This can be
//MJM: compiled completely away (if its not virtual, of course).
ACE_INLINE void
TAO::DCPS::DataLink::send(TransportQueueElement* element)
{
  DBG_ENTRY("DataLink","send");

  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.
  this->send_strategy_->send(element);
}


ACE_INLINE void
TAO::DCPS::DataLink::send_stop()
{
  DBG_ENTRY("DataLink","send_stop");

  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.
  this->send_strategy_->send_stop();
}


ACE_INLINE int
TAO::DCPS::DataLink::remove_sample(const DataSampleListElement* sample, 
                                   bool dropped_by_transport)
{
  DBG_ENTRY("DataLink","remove_sample");
  ACE_UNUSED_ARG (dropped_by_transport);

  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.
  return this->send_strategy_->remove_sample(sample);
}


ACE_INLINE void
TAO::DCPS::DataLink::remove_all_control_msgs(RepoId pub_id)
{
  DBG_ENTRY("DataLink","remove_all_control_msgs");

  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.
  this->send_strategy_->remove_all_control_msgs(pub_id);
}

/// We use our "this" pointer for our id.  Note that the "this" pointer
/// is a DataLink* as far as we are concerned.  This *is* different (due
/// to virtual tables) than the "this" pointer when in a DataLink subclass.
/// But since this is the only place where a DataLink provides its "id",
/// this should all work out (comparing ids for equality/inequality).
ACE_INLINE TAO::DCPS::DataLinkIdType
TAO::DCPS::DataLink::id() const
{
  DBG_ENTRY("DataLink","id");
  return id_;
}


ACE_INLINE int
TAO::DCPS::DataLink::start(TransportSendStrategy*    send_strategy,
                           TransportReceiveStrategy* receive_strategy)
{
  DBG_ENTRY("DataLink","start");

  // We assume that the send_strategy is not NULL, but the receive_strategy
  // is allowed to be NULL.

  // Keep a "copy" of the send_strategy.
  send_strategy->_add_ref();
  TransportSendStrategy_rch ss = send_strategy;

  // Keep a "copy" of the receive_strategy (if there is one).
  TransportReceiveStrategy_rch rs;

  if (receive_strategy != 0)
    {
      receive_strategy->_add_ref();
      rs = receive_strategy;
    }

  // Attempt to start the strategies, and if there is a start() failure,
  // make sure to stop() any strategy that was already start()'ed.
  if (ss->start() != 0)
    {
      // Failed to start the TransportSendStrategy.
      return -1;
    }

  if ((!rs.is_nil()) && (rs->start() != 0))
    {
      // Failed to start the TransportReceiveStrategy.

      // Remember to stop() the TransportSendStrategy since we did start it,
      // and now need to "undo" that action.
      ss->stop();

      return -1;
    }

  // We started both strategy objects.  Save them to data members since
  // we will now take ownership of them.
  this->send_strategy_    = ss._retn();
  this->receive_strategy_ = rs._retn();

  return 0;
}


ACE_INLINE
const char* 
TAO::DCPS::DataLink::connection_notice_as_str (enum ConnectionNotice notice)
{
  static const char* NoticeStr[] = { "DISCONNECTED",
                                     "RECONNECTED",
                                     "LOST"
                                   };

  return NoticeStr [notice];
}


