// -*- C++ -*-
//
// $Id$

#include "DataView.h"

ACE_INLINE
OpenDDS::DCPS::DataView::DataView(ACE_Message_Block& mb, size_t max_size)
  : mb_(mb)
  , max_size_(max_size)
{
}

ACE_INLINE
void
OpenDDS::DCPS::DataView::get(View& packets)
{
  ACE_Message_Block* mbptr = &mb_;
  if (mbptr == 0)
  {
    return;
  }
  char* end = mbptr->rd_ptr() + mbptr->size();
  while (mbptr != 0)
  {
    size_t size = end - mbptr->rd_ptr();
    if ((max_size_ != 0) && (size > max_size_))
    {
      size = max_size_;
    }
    packets.push_back(std::make_pair(mbptr->rd_ptr(), size));
    mbptr->rd_ptr(size);
    if (mbptr->rd_ptr() == end)
    {
      mbptr = mbptr->cont();
    }
  }
}
