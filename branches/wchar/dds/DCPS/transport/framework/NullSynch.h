// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_NULLSYNCH_H
#define OPENDDS_DCPS_NULLSYNCH_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynch.h"


namespace OpenDDS
{
  namespace DCPS
  {

    class OpenDDS_Dcps_Export NullSynch : public ThreadSynch
    {
      public:

        NullSynch(ThreadSynchResource* resource);
        virtual ~NullSynch();

        virtual void work_available();
    };

  } /* namespace DCPS */
} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "NullSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_NULLSYNCH_H */
