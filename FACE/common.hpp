#ifndef FACE_COMMON_HPP_HEADER_FILE
#define FACE_COMMON_HPP_HEADER_FILE
#include "types.hpp"

namespace FACE {

// This type is used to represent 64-bit signed integer with a
// 1 nanosecond resolution.
typedef LongLong SYSTEM_TIME_TYPE;

// This type is used to represent an infinitely long time value.
// It is often used to specify that the caller is willing to wait
// forever for an operation to complete and does not wish to timeout.
static const LongLong INF_TIME_VALUE = -1;

typedef Char CONFIGURATION_RESOURCE[256];

// FACE TS uses 'NO_ERROR' as the enumerator but this is pretty generic
// and happens to be defined as a macro on some platforms.
#if defined OPENDDS_FACE_STRICT
#define RC_NO_ERROR NO_ERROR
#endif

// This enumeration defines the possible set of status codes which may be
// returned by a method defined in the FACE API.
enum RETURN_CODE_TYPE {
  // request valid and operation performed
  RC_NO_ERROR,
  // status of system unaffected by request
  NO_ACTION,
  // no message was available
  NOT_AVAILABLE,
  // address currently in use
  ADDR_IN_USE,
  // invalid parameter specified in request
  INVALID_PARAM,
  // parameter incompatible with configuration
  INVALID_CONFIG,
  // no permission to send or connecting to wrong partition
  PERMISSION_DENIED,
  // request incompatible with current mode
  INVALID_MODE,
  // the time expired before the request could be filled
  TIMED_OUT,
  // current time - timestamp exceeds configured limits
  MESSAGE_STALE,
  // asynchronous connection in progress
  CONNECTION_IN_PROGRESS,
  // connection was closed
  CONNECTION_CLOSED,
  // Data Buffer was too small for message
  DATA_BUFFER_TOO_SMALL
};

// This type is used to represent a system address.
//
// Note: This is expected to be a pointer type such as (void *) in C.
typedef void* SYSTEM_ADDRESS_TYPE;

// This type has a one nanosecond resolution.
typedef SYSTEM_TIME_TYPE TIMEOUT_TYPE;

// This type is used to represent the message range, number of messages,
// and message type in the I/O and Transport APIs.
typedef Long MESSAGE_RANGE_TYPE;

}

#endif // FACE_COMMON_HPP_HEADER_FILE
