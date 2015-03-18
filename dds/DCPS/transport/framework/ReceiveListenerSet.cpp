/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "dds/DCPS/GuidConverter.h"

#include "ReceiveListenerSet.h"

#include <vector>

#if !defined (__ACE_INLINE__)
#include "ReceiveListenerSet.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

namespace {

  struct ReceiveListenerHandle {
    explicit ReceiveListenerHandle(TransportReceiveListener* trl)
      : listener_(trl)
    {
      if (listener_) listener_->listener_add_ref();
    }

    ReceiveListenerHandle(const ReceiveListenerHandle& rhs)
      : listener_(rhs.listener_)
    {
      if (listener_) listener_->listener_add_ref();
    }

    ReceiveListenerHandle& operator=(const ReceiveListenerHandle& rhs);

    ~ReceiveListenerHandle()
    {
      if (listener_) listener_->listener_remove_ref();
    }

  private:
    typedef void (ReceiveListenerHandle::*bool_t)() const;
    void no_implicit_conversion() const {}

  public:
    operator bool_t() const
    {
      return listener_ ? &ReceiveListenerHandle::no_implicit_conversion : 0;
    }

    TransportReceiveListener* operator->() const
    {
      return listener_;
    }

    TransportReceiveListener* listener_;
  };

  void swap(ReceiveListenerHandle& lhs, ReceiveListenerHandle& rhs)
  {
    std::swap(lhs.listener_, rhs.listener_);
  }

  ReceiveListenerHandle&
  ReceiveListenerHandle::operator=(const ReceiveListenerHandle& rhs)
  {
    ReceiveListenerHandle cpy(rhs);
    swap(*this, cpy);
    return *this;
  }
}

ReceiveListenerSet::~ReceiveListenerSet()
{
  DBG_ENTRY_LVL("ReceiveListenerSet","~ReceiveListenerSet",6);
}

bool
ReceiveListenerSet::exist(const RepoId& local_id, bool& last)
{
  GuardType guard(this->lock_);

  last = true;

  TransportReceiveListener* listener = 0;

  if (find(map_, local_id, listener) == -1) {
    GuidConverter converter(local_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ReceiveListenerSet::exist: ")
               ACE_TEXT("could not find local %C.\n"),
               OPENDDS_STRING(converter).c_str()));

    return false;
  }

  if (listener == 0) {
    GuidConverter converter(local_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ReceiveListenerSet::exist: ")
               ACE_TEXT("listener for local %C is nil.\n"),
               OPENDDS_STRING(converter).c_str()));

    return false;
  }

  last = map_.size() == 1;
  return true;
}

void
ReceiveListenerSet::get_keys(ReaderIdSeq & ids)
{
  GuardType guard(this->lock_);

  for (MapType::iterator iter = map_.begin();
       iter != map_.end(); ++ iter) {
    push_back(ids, iter->first);
  }
}

bool
ReceiveListenerSet::exist(const RepoId& local_id)
{
  GuardType guard(this->lock_);

  TransportReceiveListener* listener = 0;
  return (find(map_, local_id, listener) == -1 ? false : true);
}

void
ReceiveListenerSet::clear()
{
  GuardType guard(this->lock_);
  this->map_.clear();
}

void
ReceiveListenerSet::data_received(const ReceivedDataSample& sample,
                                  const std::set<RepoId, GUID_tKeyLessThan>& exclude)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "data_received", 6);
  std::vector<ReceiveListenerHandle> handles;
  {
    GuardType guard(this->lock_);
    for (MapType::iterator itr = map_.begin(); itr != map_.end(); ++itr) {
      if (itr->second && exclude.count(itr->first) == 0) {
        handles.push_back(ReceiveListenerHandle(itr->second));
      }
    }
  }

  for (size_t i = 0; i < handles.size(); ++i) {
    if (i < handles.size() - 1 && sample.sample_) {
      // demarshal (in data_received()) updates the rd_ptr() of any of
      // the message blocks in the chain, so give it a duplicated chain.
      ReceivedDataSample rds(sample);
      handles[i]->data_received(rds);
    } else {
      handles[i]->data_received(sample);
    }
  }
}

void
ReceiveListenerSet::data_received(const ReceivedDataSample& sample,
                                  const RepoId& readerId)
{
  DBG_ENTRY_LVL("ReceiveListenerSet", "data_received(sample, readerId)", 6);
  ReceiveListenerHandle h(0);
  {
    GuardType guard(this->lock_);
    MapType::iterator itr = map_.find(readerId);
    if (itr != map_.end() && itr->second) {
      h = ReceiveListenerHandle(itr->second);
    }
  }
  if (h) h->data_received(sample);
}

}
}
