// -*- C++ -*-
//
// $Id$
#include  "ace/Message_Block.h"
#include  "EntryExit.h"


ACE_INLINE
TAO::DCPS::ReceivedDataSample::ReceivedDataSample()
  : sample_(0)
{
  DBG_ENTRY("ReceivedDataSample","ReceivedDataSample");
}


ACE_INLINE
TAO::DCPS::ReceivedDataSample::~ReceivedDataSample()
{
  DBG_ENTRY("ReceivedDataSample","~ReceivedDataSample");

  // Release the sample_ (ACE_Message_Block chain) back to its allocator.
  if (this->sample_ != 0)
    {
      this->sample_->release();
    }
}
