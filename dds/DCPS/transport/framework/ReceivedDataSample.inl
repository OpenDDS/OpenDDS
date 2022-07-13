/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/Message_Block.h"
#include "EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
ReceivedDataSample::ReceivedDataSample(ACE_Message_Block* payload, bool take_ownership)
  : sample_(payload)
  , owns_sample_(take_ownership)
{
  DBG_ENTRY_LVL("ReceivedDataSample", "ReceivedDataSample(payload)",6);
}

ACE_INLINE
ReceivedDataSample::ReceivedDataSample(const ReceivedDataSample& other)
  : header_(other.header_)
  , sample_(other.owns_sample_ ? ACE_Message_Block::duplicate(other.sample_.get()) : other.sample_.get())
  , owns_sample_(other.owns_sample_)
{
  DBG_ENTRY_LVL("ReceivedDataSample", "ReceivedDataSample(copy)", 6);
}

ACE_INLINE ReceivedDataSample&
ReceivedDataSample::operator=(const ReceivedDataSample& other)
{
  DBG_ENTRY_LVL("ReceivedDataSample", "operator=", 6);
  ReceivedDataSample cpy(other);
  swap(*this, cpy);
  return *this;
}

ACE_INLINE
ReceivedDataSample::~ReceivedDataSample()
{
  DBG_ENTRY_LVL("ReceivedDataSample", "~ReceivedDataSample", 6);
  if (!owns_sample_) {
    sample_.release();
  }
}

ACE_INLINE void
swap(ReceivedDataSample& a, ReceivedDataSample& b)
{
  using std::swap;
  swap(a.header_, b.header_);
  swap(a.sample_, b.sample_);
  swap(a.owns_sample_, b.owns_sample_);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
