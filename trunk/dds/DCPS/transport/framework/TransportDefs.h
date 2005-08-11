// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTDEFS_H
#define TAO_DCPS_TRANSPORTDEFS_H

#include  "ace/Basic_Types.h"
#include  "dds/DCPS/Definitions.h"
#include  "dds/DCPS/Cached_Allocator_With_Overflow_T.h"

class ACE_Message_Block ;
class ACE_Data_Block ;

/**
 * Guard the allocations for the underlying memory management of the
 * receive processing with the following:
 */
#define RECEIVE_SYNCH ACE_SYNCH_NULL_MUTEX


namespace TAO
{
  namespace DCPS
  {

    /// Identifier type for DataLink objects.
    typedef ACE_UINT32 DataLinkIdType;

    /// Return code type for send_control() operations.
    enum SendControlStatus
    {
      SEND_CONTROL_ERROR,
      SEND_CONTROL_OK
    };

    /// Return code type for attach_transport() operations.
    enum AttachStatus
    {
      ATTACH_BAD_TRANSPORT,
      ATTACH_ERROR,
      ATTACH_INCOMPATIBLE_QOS,
      ATTACH_OK
    };

// TBD - Resolve this
#if 0
    // TBD SOON - The MAX_SEND_BLOCKS may conflict with the
    //            max_samples_per_packet_ configuration!
//MJM: The constant should just be used as part of a conditional at the
//MJM: points where it is used.
#endif

    /// Controls the maximum size of the iovec array used for a send packet.
#if defined (ACE_IOV_MAX) && (ACE_IOV_MAX != 0)
    enum { MAX_SEND_BLOCKS = ACE_IOV_MAX };
#else
    enum { MAX_SEND_BLOCKS = 50 };
#endif

    // Allocators are locked since we can not restrict the thread on
    // which a release will occur.

    /// Allocators used for transport receiving logic.
    enum { RECEIVE_DATA_BUFFER_SIZE = 65536 } ;

    typedef Cached_Allocator_With_Overflow<ACE_Message_Block, RECEIVE_SYNCH>
                                              TransportMessageBlockAllocator ;

    typedef Cached_Allocator_With_Overflow<ACE_Data_Block,    RECEIVE_SYNCH>
                                              TransportDataBlockAllocator ;

    typedef Cached_Allocator_With_Overflow<
              char[RECEIVE_DATA_BUFFER_SIZE],
              RECEIVE_SYNCH>                  TransportDataAllocator ;


    /// Default TransportConfiguration settings
    enum
      {
        DEFAULT_CONFIG_QUEUE_LINKS_PER_POOL   = 10,
        DEFAULT_CONFIG_QUEUE_INITIAL_POOLS    = 5,
        DEFAULT_CONFIG_MAX_PACKET_SIZE        = 2147481599,
        DEFAULT_CONFIG_MAX_SAMPLES_PER_PACKET = 10,
        DEFAULT_CONFIG_OPTIMUM_PACKET_SIZE    = 4096
      };

  } /* namespace DCPS */

} /* namespace TAO */

#endif  /* TAO_DCPS_TRANSPORTDEFS_H */
