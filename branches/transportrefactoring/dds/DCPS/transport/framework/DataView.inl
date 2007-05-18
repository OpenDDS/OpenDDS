// -*- C++ -*-
//
// $Id$

#include "DataView.h"

ACE_INLINE
TAO::DCPS::DataView::DataView(ACE_Message_Block& mb, size_t max_size)
  : mb_(mb)
  , max_size_(max_size)
{
}

ACE_INLINE
void
TAO::DCPS::DataView::get(View& packets)
{
  ACE_Message_Block* mbptr = &mb_;
  while (mbptr != 0)
  {
    follow_next_chain(*mbptr, packets);
    mbptr = mbptr->cont();
  }
}

ACE_INLINE
void
TAO::DCPS::DataView::follow_next_chain(ACE_Message_Block& mb, View& packets)
{
  ACE_Message_Block* mbptr = &mb;
  while (mbptr != 0)
  {
    size_t size = mbptr->wr_ptr() - mbptr->rd_ptr();
    if ((max_size_ != 0) && (size > max_size_))
    {
      size = max_size_;
    }
    packets.push_back(std::make_pair(mbptr->rd_ptr(), size));
    mbptr->rd_ptr(size);
    if (mbptr->rd_ptr() == mbptr->wr_ptr())
    {
      mbptr = mbptr->next();
    }
  }
}
