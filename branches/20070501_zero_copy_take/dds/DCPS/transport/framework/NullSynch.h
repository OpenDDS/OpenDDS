// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_NULLSYNCH_H
#define TAO_DCPS_NULLSYNCH_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynch.h"


namespace TAO
{
  namespace DCPS
  {

    class TAO_DdsDcps_Export NullSynch : public ThreadSynch
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
