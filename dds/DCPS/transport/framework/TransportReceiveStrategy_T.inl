/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "TransportReassembly.h"

template<typename TH, typename DSH>
ACE_INLINE int
OpenDDS::DCPS::TransportReceiveStrategy<TH, DSH>::start()
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","start",6);
  return this->start_i();
}

template<typename TH, typename DSH>
ACE_INLINE void
OpenDDS::DCPS::TransportReceiveStrategy<TH, DSH>::stop()
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","stop",6);
  this->stop_i();
}

template<typename TH, typename DSH>
ACE_INLINE const TH&
OpenDDS::DCPS::TransportReceiveStrategy<TH, DSH>::received_header() const
{
  return this->receive_transport_header_;
}

template<typename TH, typename DSH>
ACE_INLINE TH&
OpenDDS::DCPS::TransportReceiveStrategy<TH, DSH>::received_header()
{
  return this->receive_transport_header_;
}

template<typename TH, typename DSH>
ACE_INLINE const DSH&
OpenDDS::DCPS::TransportReceiveStrategy<TH, DSH>::received_sample_header() const
{
  return this->data_sample_header_;
}

template<typename TH, typename DSH>
ACE_INLINE DSH&
OpenDDS::DCPS::TransportReceiveStrategy<TH, DSH>::received_sample_header()
{
  return this->data_sample_header_;
}

template<typename TH, typename DSH>
ACE_INLINE size_t
OpenDDS::DCPS::TransportReceiveStrategy<TH, DSH>::successor_index(size_t index) const
{
  return ++index % RECEIVE_BUFFERS;
}

template<typename TH, typename DSH>
ACE_INLINE void
OpenDDS::DCPS::TransportReceiveStrategy<TH, DSH>::relink(bool)
{
  // The subclass needs implement this function for re-establishing
  // the link upon recv failure.
}
