// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_BASICQUEUELINKCHUNK_T_H
#define TAO_DCPS_BASICQUEUELINKCHUNK_T_H

#include "BasicQueueLink_T.h"

#include "ace/OS_NS_stdlib.h"

namespace TAO
{

  namespace DCPS
  {

    template <typename T>
    struct BasicQueueLinkChunk
    {
      typedef BasicQueueLink<T> LinkType;

      BasicQueueLinkChunk(size_t chunk_size)
        : next_(0)
        {
          links_ = new LinkType[chunk_size];
        }


      ~BasicQueueLinkChunk()
        {
          delete [] links_;
        }

      /// The array of LinkType objects in this chunk.
      LinkType* links_;

      /// The next chunk (or 0 if this is the last chunk).
      BasicQueueLinkChunk<T>* next_;
    };

  }

}

#endif  /* TAO_DCPS_BASICQUEUELINKCHUNK_T_H */
