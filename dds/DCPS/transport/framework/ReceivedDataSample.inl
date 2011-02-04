/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/Message_Block.h"
#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ReceivedDataSample::ReceivedDataSample()
  : sample_(0)
{
  DBG_ENTRY_LVL("ReceivedDataSample","ReceivedDataSample",6);
}

ACE_INLINE
OpenDDS::DCPS::ReceivedDataSample::~ReceivedDataSample()
{
  DBG_ENTRY_LVL("ReceivedDataSample","~ReceivedDataSample",6);

  // Release the sample_ (ACE_Message_Block chain) back to its allocator.
  if (this->sample_ != 0) {
    this->sample_->release();
  }
}
