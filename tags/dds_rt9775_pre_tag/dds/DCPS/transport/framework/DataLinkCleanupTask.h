// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_DATALINKCLEANUP_H
#define TAO_DCPS_DATALINKCLEANUP_H

#include /**/ "ace/pre.h"

#include  "dds/DCPS/transport/framework/QueueTaskBase_T.h"
#include  "dds/DCPS/transport/framework/DataLink.h"
#include  "dds/DCPS/transport/framework/DataLink_rch.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace TAO
{

  namespace DCPS
  {
    class TransportImpl;

    /**
     * @class DataLinkCleanupTask
     *
     * @brief Active Object responsible for cleaning up DataLink resources.
     *
     */
    class DataLinkCleanupTask : public QueueTaskBase <DataLink_rch>
    {
    public:
      /// Constructor.
      DataLinkCleanupTask (TransportImpl* transportImpl);

      /// Virtual Destructor.
      virtual ~DataLinkCleanupTask ();

      /// Handle reconnect requests.
      virtual void execute (DataLink_rch& dl);

    private:
      /// The associated TransportImpl (one-one relation)
      TransportImpl *transportImpl_;

    };
  }
}

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_DATALINKCLEANUP_H */
