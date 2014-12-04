#ifndef FACE_TSS_HPP_HEADER_FILE
#define FACE_TSS_HPP_HEADER_FILE
#include "common.hpp"
#include "OpenDDS_FACE_Export.h"

#define SEND_AND_RECEIVE_MESSAGE_AND_REGISTER_READ_CALLBACK_ARE_DATATYPE_SPECIFIC

namespace FACE {

typedef char CONNECTION_NAME_TYPE[64];

typedef char CONFIGURATION_FILE_NAME[256];

typedef ACE_INT32 MESSAGE_SIZE_TYPE;

// Defined by the Transport Services configuration file
// For the TWO_WAY direction types, if a single connection
// is being used for both the request and the reply,
// SOURCE is to be used at the endpoint where the request
// originates and DESTINATION is to be used at the endpoint
// where the reply originates.
enum CONNECTION_DIRECTION_TYPE {
  SOURCE,  // maps to O_WRONLY (ARINC 653 and POSIX)
  DESTINATION,  // maps to O_RDONLY (ARINC 653 and POSIX)
  BI_DIRECTIONAL,  // maps to O_RDWR (POSIX ONLY)
  ONE_WAY_REQUEST_SOURCE,
  ONE_WAY_REQUEST_DESTINATION,
  TWO_WAY_REQUEST_SYNCHRONOUS_SOURCE,
  TWO_WAY_REQUEST_SYNCHRONOUS_DESTINATION,
  TWO_WAY_REQUEST_REPLY_ASYNCHRONOUS_SOURCE,
  TWO_WAY_REQUEST_REPLY_ASYNCHRONOUS_DESTINATION,
  NOT_DEFINED_CONNECTION_DIRECTION_TYPE
};

typedef ACE_INT64 CONNECTION_ID_TYPE;

typedef ACE_INT64 TRANSACTION_ID_TYPE;

// used to tie together request/response messages
// Defined by the Transport Services configuration file
enum CONNECTION_TYPE {
  SAMPLING_PORT,  // ARINC 653 sampling port
  QUEUING_PORT,  // ARINC 653 queuing port
  SOCKET,  // POSIX socket
  // POSIX message queue is between POSIX processes belonging to the same
  // partition
  MQ,  // POSIX queue
  // POSIX shared memory is between POSIX processes belonging to the same
  // partition
  SHM,  // POSIX shared memory
  CORBA,  // CORBA
  DDS // Data Distribution Services
};

typedef bool WAITSET_TYPE[32];

struct FACE_SPECIFIED_TYPE {
  void* structure_placeholder;
};

namespace Read_Callback {
  void send_event(
    /* in */ FACE_SPECIFIED_TYPE& message,
    /* in */ MESSAGE_SIZE_TYPE message_size,
    /* in */ TRANSACTION_ID_TYPE transaction_id,
    /* in */ const WAITSET_TYPE waitset);

} // namespace Read_Callback

// Defined by the Transport Services configuration file
enum QUEUING_DISCIPLINE_TYPE {
  FIFO,
  PRIORITY,
  NOT_DEFINED_QUEUING_DISCIPLINE_TYPE
};

// Defined by the Transport Services configuration file
enum CONNECTION_DOMAIN_TYPE {
  UNIX,  // AF_UNIX
  INET,  // AF_INET
  NOT_DEFINED_CONNECTION_DOMAIN_TYPE
};

// Defined by the Transport Services configuration file
enum SOCKET_TYPE {
  STREAM,  // SOCK_STREAM
  DGRAM,  // SOCK_DGRAM
  SEQPACKET,  // SOCK_SEQPACKET
  NOT_DEFINED_SOCKET_TYPE
};

// Defined by the Transport Services configuration file
enum RECEIVE_FLAG_TYPE {
  PEEK,  // SOCK_STREAM
  OOB_RECEIVE_FLAG_TYPE,  // SOCK_DGRAM
  WAITALL,  // SOCK_SEQPACKET
  NOT_DEFINED_RECEIVE_FLAG_TYPE
};

// Defined by the Transport Services configuration file
enum SEND_FLAG_TYPE {
  EOR,  // SOCK_STREAM
  OOB_SEND_FLAG_TYPE,  // SOCK_DGRAM
  NOSIGNAL,  // SOCK_SEQPACKET
  NOT_DEFINED_SEND_FLAG_TYPE
};

enum VALIDITY_TYPE {
  INVALID,  // Also "empty" and "down"
  VALID // Also "occupied" and "up"
};

// Addressed in Validity_Type
// typedef enum { EMPTY = 0, OCCUPIED = 1} EMPTY_INDICATOR_TYPE;
// typedef enum { DOWN = 0, UP = 1} EVENT_STATE_TYPE;

// Support publish/subscribe and request/response
enum MESSAGING_PATTERN_TYPE {
  PUB_SUB,
  CLIENT,
  SERVER
};

typedef ACE_INT32 WAITING_RANGE_TYPE;

// Elements defined by the Transport Services configuration file
struct TRANSPORT_CONNECTION_STATUS_TYPE {
  // Elements defined by the Transport Services configuration file
  MESSAGE_RANGE_TYPE MESSAGE;
  // Elements defined by the Transport Services configuration file
  MESSAGE_RANGE_TYPE MAX_MESSAGE;
  // Elements defined by the Transport Services configuration file
  MESSAGE_SIZE_TYPE MAX_MESSAGE_SIZE;
  // Elements defined by the Transport Services configuration file
  CONNECTION_DIRECTION_TYPE CONNECTION_DIRECTION;
  // Elements defined by the Transport Services configuration file
  WAITING_RANGE_TYPE WAITING_PROCESSES_OR_MESSAGES;
  // Elements defined by the Transport Services configuration file
  SYSTEM_TIME_TYPE REFRESH_PERIOD;
  // Elements defined by the Transport Services configuration file
  VALIDITY_TYPE LAST_MSG_VALIDITY;
};

namespace TS {
  OpenDDS_FACE_Export
  void Initialize(
    /* in */ const CONFIGURATION_FILE_NAME configuration_file,
    /* out */ RETURN_CODE_TYPE& return_code);

  OpenDDS_FACE_Export
  void create_connection(
    /* in */ const CONNECTION_NAME_TYPE connection_name,
    /* in */ MESSAGING_PATTERN_TYPE pattern,
    /* out */ CONNECTION_ID_TYPE& connection_id,
    /* out */ CONNECTION_DIRECTION_TYPE& connection_direction,
    /* out */ MESSAGE_SIZE_TYPE& max_message_size,
    /* out */ RETURN_CODE_TYPE& return_code);

  OpenDDS_FACE_Export
  void destroy_connection(
    /* in */ CONNECTION_ID_TYPE connection_id,
    /* out */ RETURN_CODE_TYPE& return_code);

#ifndef SEND_AND_RECEIVE_MESSAGE_AND_REGISTER_READ_CALLBACK_ARE_DATATYPE_SPECIFIC
  void receive_message(
    /* in */ CONNECTION_ID_TYPE connection_id,
    /* in */ TIMEOUT_TYPE timeout,
    /* inout */ TRANSACTION_ID_TYPE& transaction_id,
    /* out */ FACE_SPECIFIED_TYPE& message,
    /* in */ MESSAGE_SIZE_TYPE message_size,
    /* out */ RETURN_CODE_TYPE& return_code);

  void send_message(
    /* in */ CONNECTION_ID_TYPE connection_id,
    /* in */ FACE_SPECIFIED_TYPE& message,
    /* inout */ MESSAGE_SIZE_TYPE& message_size,
    /* in */ TIMEOUT_TYPE timeout,
    /* inout */ TRANSACTION_ID_TYPE& transaction_id,
    /* out */ RETURN_CODE_TYPE& return_code);

  void register_read_callback(
    /* in */ CONNECTION_ID_TYPE connection_id,
    /* in */ const WAITSET_TYPE waitset,
    /* in */ const Read_Callback& data_callback,
    /* in */ MESSAGE_SIZE_TYPE max_message_size,
    /* out */ RETURN_CODE_TYPE& return_code);
#endif

  OpenDDS_FACE_Export
  void get_connection_parameters(
    /* inout */ CONNECTION_NAME_TYPE& connection_name,
    /* inout */ CONNECTION_ID_TYPE& connection_id,
    /* out */ TRANSPORT_CONNECTION_STATUS_TYPE& connection_status,
    /* out */ RETURN_CODE_TYPE& return_code);

} // namespace TS

} // namespace FACE

#endif // FACE_TSS_HPP_HEADER_FILE
