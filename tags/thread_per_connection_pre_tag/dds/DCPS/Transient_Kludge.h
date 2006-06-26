// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_TRANSIENT_KLUDGE_H
#define TAO_DDS_DCPS_TRANSIENT_KLUDGE_H


#include "dcps_export.h"
#include "tao/TAO_Singleton.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO
{
  namespace DCPS
  {
    /**
    * @class TransientKludge
    *
    * @brief Simply turn on and off the transient kludge enable flag.
    *
    * This class provides the methods to set/get transient kludge
    * enable flag.
    * Only the DCPSInfo repository should set/enable the kludge!!!
    */
    class TAO_DdsDcps_Export Transient_Kludge
    {
    public:

      Transient_Kludge ();
      ~Transient_Kludge ();

      /// Turn on enabled_ flag.
      void enable ();

      /// Turn off enabled_ flag.
      void disable ();

      /// Accessor of enable flag.
      bool is_enabled ();

    private:
      /// The flag.
      bool  enabled_;
    };

    typedef TAO_Singleton< ::TAO::DCPS::Transient_Kludge, TAO_SYNCH_MUTEX > TRANSIENT_KLUDGE;

    TAO_DDSDCPS_SINGLETON_DECLARE (::TAO_Singleton,
                                   Transient_Kludge,
                                   TAO_SYNCH_MUTEX)

    #define TheTransientKludge TRANSIENT_KLUDGE::instance()

  } // namespace DCPS
} // namespace TAO

#if defined(__ACE_INLINE__)
#include "Transient_Kludge.inl"
#endif /* __ACE_INLINE__ */


#endif /* TAO_DDS_DCPS_TRANSIENT_KLUDGE_H */
