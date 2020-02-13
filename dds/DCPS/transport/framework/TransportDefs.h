/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTDEFS_H
#define OPENDDS_DCPS_TRANSPORTDEFS_H

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/Cached_Allocator_With_Overflow_T.h"
#include "dds/DCPS/debug.h"
#include "ace/Basic_Types.h"
#include "ace/CDR_Base.h"
#include "ace/Synch_Traits.h"
#include "TransportDebug.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
class ACE_Data_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

/**
 * Guard the allocations for the underlying memory management of the
 * receive processing with the following:
 *
 * Notice that even we have only one thread for receiving per transport, the underly message blocks
 * would interact with the threads from EndHistoricSamplesMissedSweeper which are different from the
 * receiving threads. Therefore, we cannot use ACE_SYNCH_NULL_MUTEX here.
 */
#define RECEIVE_SYNCH ACE_SYNCH_MUTEX

/// Macro to get the individual configuration value
///  from ACE_Configuration_Heap and cast to the specific
///  type from integer.
#define GET_CONFIG_VALUE(CF, SECT, KEY, VALUE, TYPE)                             \
  {                                                                              \
    ACE_TString stringvalue;                                                     \
    if (CF.get_string_value (SECT, KEY, stringvalue) == -1)                      \
    {                                                                            \
      if (OpenDDS::DCPS::Transport_debug_level > 0)                            \
      {                                                                          \
        ACE_DEBUG ((LM_NOTICE,                                                   \
                    ACE_TEXT ("(%P|%t) NOTICE: \"%s\" is not defined in config ")\
                    ACE_TEXT ("file - using code default.\n"),                   \
                    KEY));                                                       \
      }                                                                          \
    }                                                                            \
    else  if (stringvalue == ACE_TEXT(""))                                       \
    {                                                                            \
      if (OpenDDS::DCPS::Transport_debug_level > 0)                            \
      {                                                                          \
        ACE_DEBUG ((LM_WARNING,                                                  \
                    ACE_TEXT ("(%P|%t) WARNING: \"%s\" is defined in config ")   \
                    ACE_TEXT ("file, but is missing value - using code default.\n"), \
                    KEY));                                                       \
      }                                                                          \
    }                                                                            \
    else                                                                         \
    {                                                                            \
      VALUE = static_cast<TYPE>(ACE_OS::atoi (stringvalue.c_str()));             \
    }                                                                            \
  }

/// Macro to get the individual configuration value
///  from ACE_Configuration_Heap as string type.
#define GET_CONFIG_STRING_VALUE(CF, SECT, KEY, VALUE)                            \
  {                                                                              \
    ACE_TString stringvalue;                                                     \
    if (CF.get_string_value (SECT, KEY, stringvalue) == -1)                      \
    {                                                                            \
      if (OpenDDS::DCPS::Transport_debug_level > 0)                              \
      {                                                                          \
        ACE_DEBUG ((LM_NOTICE,                                                   \
                    ACE_TEXT ("(%P|%t) NOTICE: \"%s\" is not defined in config ") \
                    ACE_TEXT ("file - using code default.\n"),                   \
                    KEY));                                                       \
      }                                                                          \
    }                                                                            \
    else if (stringvalue == ACE_TEXT(""))                                       \
    {                                                                            \
      if (OpenDDS::DCPS::Transport_debug_level > 0)                            \
      {                                                                          \
        ACE_DEBUG ((LM_WARNING,                                                  \
                    ACE_TEXT ("(%P|%t) WARNING: \"%s\" is defined in config ")   \
                    ACE_TEXT ("file, but is missing value - using code default.\n"), \
                    KEY));                                                       \
      }                                                                          \
    }                                                                            \
    else                                                                         \
    {                                                                            \
      VALUE = ACE_TEXT_ALWAYS_CHAR(stringvalue.c_str());                         \
    }                                                                            \
  }

#define GET_CONFIG_TSTRING_VALUE(CF, SECT, KEY, VALUE)                           \
  {                                                                              \
    ACE_TString stringvalue;                                                     \
    if (CF.get_string_value (SECT, KEY, stringvalue) == -1)                      \
    {                                                                            \
      if (OpenDDS::DCPS::Transport_debug_level > 0)                              \
      {                                                                          \
        ACE_DEBUG ((LM_NOTICE,                                                   \
                    ACE_TEXT ("(%P|%t) NOTICE: \"%s\" is not defined in config ") \
                    ACE_TEXT ("file - using code default.\n"),                   \
                    KEY));                                                       \
      }                                                                          \
    }                                                                            \
    else if (stringvalue == ACE_TEXT(""))                                        \
    {                                                                            \
      if (OpenDDS::DCPS::Transport_debug_level > 0)                              \
      {                                                                          \
        ACE_DEBUG ((LM_WARNING,                                                  \
                    ACE_TEXT ("(%P|%t) WARNING: \"%s\" is defined in config ")   \
                    ACE_TEXT ("file, but is missing value - using code default.\n"), \
                    KEY));                                                       \
      }                                                                          \
    }                                                                            \
    else                                                                         \
    {                                                                            \
      VALUE = stringvalue;                                                       \
    }                                                                            \
  }

#define GET_CONFIG_DOUBLE_VALUE(CF, SECT, KEY, VALUE)                            \
  {                                                                              \
    ACE_TString stringvalue;                                                     \
    if (CF.get_string_value (SECT, KEY, stringvalue) == -1)                      \
    {                                                                            \
      if (OpenDDS::DCPS::Transport_debug_level > 0)                            \
      {                                                                          \
        ACE_DEBUG ((LM_NOTICE,                                                   \
                    ACE_TEXT ("(%P|%t) NOTICE: \"%s\" is not defined in config ") \
                    ACE_TEXT ("file - using code default.\n"),                   \
                    KEY));                                                       \
      }                                                                          \
    }                                                                            \
    else  if (stringvalue == ACE_TEXT(""))                                       \
    {                                                                            \
      if (OpenDDS::DCPS::Transport_debug_level > 0)                            \
      {                                                                          \
        ACE_DEBUG ((LM_WARNING,                                                  \
                    ACE_TEXT ("(%P|%t) WARNING: \"%s\" is defined in config ")   \
                    ACE_TEXT ("file, but is missing value - using code default.\n"), \
                    KEY));                                                       \
      }                                                                          \
    }                                                                            \
    else                                                                         \
    {                                                                            \
      VALUE = ACE_OS::strtod (stringvalue.c_str(), 0);                           \
    }                                                                            \
  }

/// Macro to get the individual configuration value
///  from ACE_Configuration_Heap as TimeDuration
///  using milliseconds.
#define GET_CONFIG_TIME_VALUE(CF, SECT, KEY, VALUE)                              \
  {                                                                              \
    long tv = -1;                                                                \
    GET_CONFIG_VALUE(CF, SECT, KEY, tv, long);                                   \
    if (tv != -1) VALUE = TimeDuration::from_msec(tv); \
  }

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// The transport and configuration section name used in the config file format
static const ACE_TCHAR TRANSPORT_SECTION_NAME[] = ACE_TEXT("transport");
static const ACE_TCHAR CONFIG_SECTION_NAME[] = ACE_TEXT("config");

/// Identifier type for DataLink objects.
typedef ACE_UINT64  DataLinkIdType;

/// Return code type for send_control() operations.
enum SendControlStatus {
  SEND_CONTROL_ERROR,
  SEND_CONTROL_OK
};

/// Return code type for attach_transport() operations.
enum AttachStatus {
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
enum { RECEIVE_DATA_BUFFER_SIZE = 65536 };

typedef Cached_Allocator_With_Overflow<ACE_Message_Block, RECEIVE_SYNCH>
  TransportMessageBlockAllocator;

typedef Cached_Allocator_With_Overflow<ACE_Data_Block,    RECEIVE_SYNCH>
  TransportDataBlockAllocator;

typedef Cached_Allocator_With_Overflow<char[RECEIVE_DATA_BUFFER_SIZE],
                                       RECEIVE_SYNCH>
  TransportDataAllocator;

/// Default TransportInst settings
enum {
  DEFAULT_CONFIG_QUEUE_MESSAGES_PER_POOL = 10,
  DEFAULT_CONFIG_QUEUE_INITIAL_POOLS     = 5,
  DEFAULT_CONFIG_MAX_PACKET_SIZE         = 2147481599,
  DEFAULT_CONFIG_MAX_SAMPLES_PER_PACKET  = 10,
  DEFAULT_CONFIG_OPTIMUM_PACKET_SIZE     = 4096
};

/// used by DataLink::remove_sample(), TransportSendStrategy, *RemoveVisitor
enum RemoveResult {
  REMOVE_ERROR, REMOVE_NOT_FOUND, REMOVE_FOUND, REMOVE_RELEASED
};

typedef ACE_CDR::Long Priority;

typedef size_t ConnectionInfoFlags;
static const ConnectionInfoFlags CONNINFO_UNICAST = (1 << 0);
static const ConnectionInfoFlags CONNINFO_MULTICAST = (1 << 1);
static const ConnectionInfoFlags CONNINFO_ALL = static_cast<ConnectionInfoFlags>(-1);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_TRANSPORTDEFS_H */
