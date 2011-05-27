// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_LINKIMPLCALLBACK_H
#define OPENDDS_DCPS_LINKIMPLCALLBACK_H

#include "dds/DCPS/dcps_export.h"
#include <ace/Message_Block.h>

namespace OpenDDS
{
  namespace DCPS
  {

    /**
     * This class represents the LinkImplCallback which should be
     * implemented by those who make requests of the LinkImpl class.
     */
    class OpenDDS_Dcps_Export LinkImplCallback
    {
    public:
      LinkImplCallback() {}
      virtual ~LinkImplCallback() {}

      virtual void receivedData(const ACE_Message_Block& mb) = 0;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#endif  /* OPENDDS_DCPS_LINKIMPLCALLBACK_H */
