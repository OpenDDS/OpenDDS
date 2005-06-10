// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_NULLSYNCH_H
#define TAO_DCPS_NULLSYNCH_H

#include  "ThreadSynch.h"


namespace TAO
{
  namespace DCPS
  {

    class NullSynch : public ThreadSynch
    {
      public:

        NullSynch(ThreadSynchResource* resource);
        virtual ~NullSynch();

        virtual void work_available();
    };

  } /* namespace DCPS */
} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "NullSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_NULLSYNCH_H */
