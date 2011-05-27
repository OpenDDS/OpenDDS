// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPTRANSPORT_H
#define TAO_DCPS_SIMPLEUDPTRANSPORT_H

#include  "SimpleUnreliableDgram_export.h"
#include  "SimpleUnreliableDgramTransport.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleUdpTransport : public SimpleUnreliableDgramTransport
    {
      public:

        SimpleUdpTransport();
        virtual ~SimpleUdpTransport();

      protected:

        virtual int configure_socket(TransportConfiguration* config);

        virtual int connection_info_i
                                 (TransportInterfaceInfo& local_info) const;

        virtual void deliver_sample(ReceivedDataSample&  sample,
                            const ACE_INET_Addr& remote_address);

      private:

        ACE_INET_Addr local_address_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUdpTransport.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLEUDPTRANSPORT_H */
