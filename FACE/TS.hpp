#ifndef FACE_TS_HPP_HEADER_FILE
#define FACE_TS_HPP_HEADER_FILE
#include "common.hpp"
#include "TS_common.hpp"
#include "OpenDDS_FACE_Export.h"

namespace FACE {
namespace TS {

  OpenDDS_FACE_Export
  void Initialize(
    /* in */ const CONFIGURATION_RESOURCE configuration_file,
    /* out */ RETURN_CODE_TYPE& return_code);

  OpenDDS_FACE_Export
  void Create_Connection(
    /* in */ const CONNECTION_NAME_TYPE connection_name,
    /* in */ MESSAGING_PATTERN_TYPE pattern,
    /* out */ CONNECTION_ID_TYPE& connection_id,
    /* out */ CONNECTION_DIRECTION_TYPE& connection_direction,
    /* out */ MESSAGE_SIZE_TYPE& max_message_size,
    /* in */ TIMEOUT_TYPE timeout,
    /* out */ RETURN_CODE_TYPE& return_code);

  OpenDDS_FACE_Export
  void Destroy_Connection(
    /* in */ CONNECTION_ID_TYPE connection_id,
    /* out */ RETURN_CODE_TYPE& return_code);

  OpenDDS_FACE_Export
  void Unregister_Callback(
    /* in */ CONNECTION_ID_TYPE connection_id,
    /* out */ RETURN_CODE_TYPE& return_code);

  OpenDDS_FACE_Export
  void Get_Connection_Parameters(
    /* inout */ CONNECTION_NAME_TYPE& connection_name,
    /* inout (0 to specify out)*/ CONNECTION_ID_TYPE& connection_id,
    /* out */ TRANSPORT_CONNECTION_STATUS_TYPE& connection_status,
    /* out */ RETURN_CODE_TYPE& return_code);

  OpenDDS_FACE_Export
  void Receive_Message(
    /* in */ CONNECTION_ID_TYPE connection_id,
    /* in */ TIMEOUT_TYPE timeout,
    /* inout */ TRANSACTION_ID_TYPE& transaction_id,
    /* out */ MessageHeader& message_header,
    /* in */ MESSAGE_SIZE_TYPE message_size,
    /* out */ RETURN_CODE_TYPE& return_code);
} // namespace TS
} // namespace FACE

#endif // FACE_TS_HPP_HEADER_FILE
