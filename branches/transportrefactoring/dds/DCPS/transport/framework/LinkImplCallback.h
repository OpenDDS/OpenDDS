// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_LINKIMPLCALLBACK_H
#define TAO_DCPS_LINKIMPLCALLBACK_H

#include "dds/DCPS/dcps_export.h"
#include <ace/Message_Block.h>

namespace TAO
{
  namespace DCPS
  {

    /**
     * This class represents the LinkImplCallback which should be
     * implemented by those who make requests of the LinkImpl class.
     */
    class TAO_DdsDcps_Export LinkImplCallback
    {
    public:
      LinkImplCallback() {}
      virtual ~LinkImplCallback() {}

      virtual void receivedData(const ACE_Message_Block& mb) = 0;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#endif  /* TAO_DCPS_LINKIMPLCALLBACK_H */
