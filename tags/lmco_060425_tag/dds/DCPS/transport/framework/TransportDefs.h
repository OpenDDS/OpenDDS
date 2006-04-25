// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTDEFS_H
#define TAO_DCPS_TRANSPORTDEFS_H

#include  "ace/Basic_Types.h"
#include  "dds/DCPS/Definitions.h"
#include  "dds/DCPS/Cached_Allocator_With_Overflow_T.h"
#include  "dds/DCPS/Definitions.h"
#include  "dds/DCPS/debug.h"

class ACE_Message_Block ;
class ACE_Data_Block ;

/**
 * Guard the allocations for the underlying memory management of the
 * receive processing with the following:
 */
#define RECEIVE_SYNCH ACE_SYNCH_NULL_MUTEX

/// Macro to get the individual configuration value from ACE_Configuration_Heap and cast to the specific
/// type from integer.
#define GET_CONFIG_VALUE(CF, SECT, KEY, VALUE, TYPE)                           \
{                                                                              \
  ACE_CString stringvalue;                                                     \
  if (CF.get_string_value (SECT, KEY, stringvalue) == -1)                      \
  {                                                                            \
    if (DCPS_debug_level > 0)                                                  \
    {                                                                          \
      ACE_DEBUG ((LM_WARNING,                                                  \
                  ACE_TEXT ("(%P|%t)\"%s\" is not defined in config file - using code default.\n"),\
                  KEY));                                                       \
    }                                                                          \
  }                                                                            \
  else  if (stringvalue == "")                                                 \
  {                                                                            \
    if (DCPS_debug_level > 0)                                                  \
    {                                                                          \
      ACE_DEBUG ((LM_WARNING,                                                  \
                ACE_TEXT ("(%P|%t)missing VALUE for \"%s\" in config file - using code default.\n"),\
                KEY));                                                         \
    }                                                                          \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    VALUE = static_cast<TYPE>(ACE_OS::atoi (stringvalue.c_str ()));            \
  }                                                                            \
}

/// Macro to get the individual configuration value from ACE_Configuration_Heap as string type.
#define GET_CONFIG_STRING_VALUE(CF, SECT, KEY, VALUE)                          \
{                                                                              \
  ACE_CString stringvalue;                                                     \
  if (CF.get_string_value (SECT, KEY, stringvalue) == -1)                      \
  {                                                                            \
    if (DCPS_debug_level > 0)                                                  \
    {                                                                          \
      ACE_DEBUG ((LM_WARNING,                                                  \
                  ACE_TEXT ("(%P|%t)\"%s\" is not defined in config file - using code default.\n"),\
                  KEY));                                                       \
    }                                                                          \
  }                                                                            \
  else  if (stringvalue == "")                                                 \
  {                                                                            \
    if (DCPS_debug_level > 0)                                                  \
    {                                                                          \
      ACE_DEBUG ((LM_WARNING,                                                  \
                  ACE_TEXT ("(%P|%t)missing VALUE for \"%s\" in config file - using code default.\n"),\
                  KEY));                                                       \
    }                                                                          \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    VALUE = stringvalue;                                                       \
  }                                                                            \
}

#define GET_CONFIG_DOUBLE_VALUE(CF, SECT, KEY, VALUE)                           \
{                                                                              \
  ACE_CString stringvalue;                                                     \
  if (CF.get_string_value (SECT, KEY, stringvalue) == -1)                      \
  {                                                                            \
    if (DCPS_debug_level > 0)                                                  \
    {                                                                          \
      ACE_DEBUG ((LM_WARNING,                                                  \
                  ACE_TEXT ("(%P|%t)\"%s\" is not defined in config file - using code default.\n"),\
                  KEY));                                                       \
    }                                                                          \
  }                                                                            \
  else  if (stringvalue == "")                                                 \
  {                                                                            \
    if (DCPS_debug_level > 0)                                                  \
    {                                                                          \
      ACE_DEBUG ((LM_WARNING,                                                  \
                ACE_TEXT ("(%P|%t)missing VALUE for \"%s\" in config file - using code default.\n"),\
                KEY));                                                         \
    }                                                                          \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    VALUE = atof (stringvalue.c_str ());                                       \
  }                                                                            \
}

// The factory section name prefix.
static const char FACTORY_SECTION_NAME_PREFIX[] = "transport_factory_";
// The factory section name prefix is "transport_factory_" so the length is 18.
static const size_t FACTORY_SECTION_NAME_PREFIX_LEN = ACE_OS::strlen (FACTORY_SECTION_NAME_PREFIX);
// The transport section name prefix.
static const char  TRANSPORT_SECTION_NAME_PREFIX[] = "transport_impl_";
// The transport section name prefix is "transport_impl_" so the length is 15.
static const size_t TRANSPORT_SECTION_NAME_PREFIX_LEN = ACE_OS::strlen (TRANSPORT_SECTION_NAME_PREFIX); 

namespace TAO
{
  namespace DCPS
  {
    // Values used in TransportFactory::create_transport_impl () call.
    const bool AUTO_CONFIG = 1;
    const bool DONT_AUTO_CONFIG = 0;

    /// The TransportImplFactory instance ID type.
    typedef ACE_CString FactoryIdType;

    /// The TransportImpl instance ID type.
    typedef ACE_UINT32 TransportIdType;

    /// Identifier type for DataLink objects.
    typedef ACE_UINT64  DataLinkIdType;

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
        DEFAULT_CONFIG_QUEUE_MESSAGES_PER_POOL   = 10,
        DEFAULT_CONFIG_QUEUE_INITIAL_POOLS    = 5,
        DEFAULT_CONFIG_MAX_PACKET_SIZE        = 2147481599,
        DEFAULT_CONFIG_MAX_SAMPLES_PER_PACKET = 10,
        DEFAULT_CONFIG_OPTIMUM_PACKET_SIZE    = 4096
      };

  } /* namespace DCPS */

} /* namespace TAO */

#endif  /* TAO_DCPS_TRANSPORTDEFS_H */
