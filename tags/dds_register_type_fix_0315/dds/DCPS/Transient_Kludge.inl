// -*- C++ -*-
//
// $Id$


ACE_INLINE
TAO::DCPS::Transient_Kludge::Transient_Kludge ()
: enabled_ (false)
{
}


ACE_INLINE
TAO::DCPS::Transient_Kludge::~Transient_Kludge ()
{
}


ACE_INLINE
void 
TAO::DCPS::Transient_Kludge::enable ()
{
  enabled_ = true;
}


ACE_INLINE
void 
TAO::DCPS::Transient_Kludge::disable ()
{
  enabled_ = false;
}


ACE_INLINE
bool 
TAO::DCPS::Transient_Kludge::is_enabled ()
{
  return enabled_;
}



