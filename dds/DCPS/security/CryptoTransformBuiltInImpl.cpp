#include "dds/DCPS/security/CryptoTransformBuiltInImpl.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DdsDcpsInfrastructureC.h"

namespace OpenDDS {
namespace Security {

::CORBA::Boolean CryptoTransformBuiltInImpl::encode_serialized_payload(
  ::DDS::OctetSeq & encoded_buffer,
  ::DDS::OctetSeq & extra_inline_qos,
  const ::DDS::OctetSeq & plain_buffer,
  ::DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
  ::DDS::Security::SecurityException & ex)
  {
    ACE_UNUSED_ARG(extra_inline_qos);

    if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid datawriter handle");
      return false;
    }

    // Simple implementation wraps the plain_buffer back into the output
    // and adds no extra_inline_qos
    ::DDS::OctetSeq transformed_buffer(plain_buffer);
    encoded_buffer.swap(transformed_buffer);
    return true;
  }

::CORBA::Boolean CryptoTransformBuiltInImpl::encode_datawriter_submessage(
  ::DDS::OctetSeq & encoded_rtps_submessage,
  const ::DDS::OctetSeq & plain_rtps_submessage,
  ::DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
  const ::DDS::Security::DatareaderCryptoHandleSeq & receiving_datareader_crypto_list,
  ::CORBA::Long & receiving_datareader_crypto_list_index,
  ::DDS::Security::SecurityException & ex)
  {
    // Verify the input handles are valid before doing the transformation
    if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid datawriter handle");
      return false;
    }

    ::DDS::Security::DatareaderCryptoHandle reader_handle = DDS::HANDLE_NIL;
    if (receiving_datareader_crypto_list_index >= 0) {
      // Need to make this unsigned to get prevent warnings when comparing to length()
      CORBA::ULong index = static_cast<CORBA::ULong>(receiving_datareader_crypto_list_index);
      if (index < receiving_datareader_crypto_list.length()) {
        reader_handle = receiving_datareader_crypto_list[receiving_datareader_crypto_list_index];
      }
    }

    if (DDS::HANDLE_NIL == reader_handle) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid datareader handle");
      return false;
    }

    // Simple implementation wraps the plain_buffer back into the output
    // and adds no extra_inline_qos
    ::DDS::OctetSeq transformed_buffer(plain_rtps_submessage);
    encoded_rtps_submessage.swap(transformed_buffer);

    // Advance the counter to indicate this reader has been handled
    ++receiving_datareader_crypto_list_index;

    return true;
  }

::CORBA::Boolean CryptoTransformBuiltInImpl::encode_datareader_submessage(
  ::DDS::OctetSeq & encoded_rtps_submessage,
  const ::DDS::OctetSeq & plain_rtps_submessage,
  ::DDS::Security::DatareaderCryptoHandle sending_datareader_crypto,
  const ::DDS::Security::DatawriterCryptoHandleSeq & receiving_datawriter_crypto_list,
  ::DDS::Security::SecurityException & ex)
  {
    // Perform sanity checking on input data
    if (DDS::HANDLE_NIL == sending_datareader_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid DataReader handle");
      return false;
    }
    if (0 == receiving_datawriter_crypto_list.length()) {
      CommonUtilities::set_security_error(ex, -1, 0, "No Datawriters specified");
      return false;
    }

    // For the stub, just copy the plain message into the encoded message
    ::DDS::OctetSeq transformed_buffer(plain_rtps_submessage);
    encoded_rtps_submessage.swap(transformed_buffer);

    return true;
  }

::CORBA::Boolean CryptoTransformBuiltInImpl::encode_rtps_message(
  ::DDS::OctetSeq & encoded_rtps_message,
  const ::DDS::OctetSeq & plain_rtps_message,
  ::DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
  const ::DDS::Security::ParticipantCryptoHandleSeq & receiving_participant_crypto_list,
  ::CORBA::Long & receiving_participant_crypto_list_index,
  ::DDS::Security::SecurityException & ex)
  {
    // Perform sanity checking on input data
    if (DDS::HANDLE_NIL == sending_participant_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid DataReader handle");
      return false;
    }
    if (0 == receiving_participant_crypto_list.length()) {
      CommonUtilities::set_security_error(ex, -1, 0, "No Datawriters specified");
      return false;
    }

    ::DDS::Security::ParticipantCryptoHandle dest_handle = DDS::HANDLE_NIL;
    if (receiving_participant_crypto_list_index >= 0) {
      // Need to make this unsigned to get prevent warnings when comparing to length()
      CORBA::ULong index = static_cast<CORBA::ULong>(receiving_participant_crypto_list_index);
      if (index < receiving_participant_crypto_list.length()) {
        dest_handle = receiving_participant_crypto_list[receiving_participant_crypto_list_index];
      }
    }

    if (DDS::HANDLE_NIL == dest_handle) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid receiver handle");
      return false;
    }

    // Simple implementation wraps the plain_buffer back into the output
    // and adds no extra_inline_qos
    ::DDS::OctetSeq transformed_buffer(plain_rtps_message);
    encoded_rtps_message.swap(transformed_buffer);

    // Advance the counter to indicate this reader has been handled
    ++receiving_participant_crypto_list_index;

    return true;
  }

::CORBA::Boolean CryptoTransformBuiltInImpl::decode_rtps_message(
  ::DDS::OctetSeq & plain_buffer,
  const ::DDS::OctetSeq & encoded_buffer,
  ::DDS::Security::ParticipantCryptoHandle receiving_participant_crypto,
  ::DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
  ::DDS::Security::SecurityException & ex)
  {
    // Perform sanity checking on input data
    if (DDS::HANDLE_NIL == receiving_participant_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Receiving Participant handle");
      return false;
    }
    if (DDS::HANDLE_NIL == sending_participant_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "No Sending Participant handle");
      return false;
    }

    // For the stub, just supply the input as the output
    ::DDS::OctetSeq transformed_buffer(encoded_buffer);
    plain_buffer.swap(transformed_buffer);

    return true;
  }

::CORBA::Boolean CryptoTransformBuiltInImpl::preprocess_secure_submsg(
  ::DDS::Security::DatawriterCryptoHandle & datawriter_crypto,
  ::DDS::Security::DatareaderCryptoHandle & datareader_crypto,
  ::DDS::Security::SecureSubmessageCategory_t & secure_submessage_category,
  const ::DDS::OctetSeq & encoded_rtps_submessage,
  ::DDS::Security::ParticipantCryptoHandle receiving_participant_crypto,
  ::DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
  ::DDS::Security::SecurityException & ex)
  {
    ACE_UNUSED_ARG(encoded_rtps_submessage);

    if (DDS::HANDLE_NIL == receiving_participant_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Receiving Participant");
      return false;
    }
    if (DDS::HANDLE_NIL == sending_participant_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Sending Participant");
      return false;
    }

    // For now, just set some simple values, but this won't
    // be very useful as a stub
    secure_submessage_category = DDS::Security::DATAWRITER_SUBMESSAGE;
    datawriter_crypto = 1;
    datareader_crypto = 2;
    
    //enum SecureSubmessageCategory_t
    //{
    //  INFO_SUBMESSAGE,
    //  DATAWRITER_SUBMESSAGE,
    //  DATAREADER_SUBMESSAGE
    //};

    // Determine submessage type

    // If DATAWRITER_SUBMESSAGE:
    //  Set datawriter_crypto to local datawriter crypto handle
    //  Set datareader_crypto to remote crypto handle linked to datawriter_crypto
    // else is DATAREADER_SUBMESSAGE:
    //  Set datareader_crypto to local datareader crypto handle
    //  Set datawriter_crypto to remote crypto handle linked to datareader_crypto
    
    // else Fail
    // Set datawriter_crypto to be either the local writer attached to the
    // external reader 
    return true;
  }

::CORBA::Boolean CryptoTransformBuiltInImpl::decode_datawriter_submessage(
  ::DDS::OctetSeq & plain_rtps_submessage,
  const ::DDS::OctetSeq & encoded_rtps_submessage,
  ::DDS::Security::DatareaderCryptoHandle receiving_datareader_crypto,
  ::DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
  const ::DDS::Security::SecurityException & ex)
  {
    // Error ex is marked as const in this function call
    ACE_UNUSED_ARG(ex);

    if (DDS::HANDLE_NIL == receiving_datareader_crypto) {
      //CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datareader handle");
      return false;
    }
    if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
      //CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datawriter handle");
      return false;
    }

    // For the stub, just supply the input as the output
    ::DDS::OctetSeq transformed_buffer(encoded_rtps_submessage);
    plain_rtps_submessage.swap(transformed_buffer);

    return true;
  }

::CORBA::Boolean CryptoTransformBuiltInImpl::decode_datareader_submessage(
  ::DDS::OctetSeq & plain_rtps_message,
  const ::DDS::OctetSeq & encoded_rtps_message,
  ::DDS::Security::DatawriterCryptoHandle receiving_datawriter_crypto,
  ::DDS::Security::DatareaderCryptoHandle sending_datareader_crypto,
  ::DDS::Security::SecurityException & ex)
  {
    if (DDS::HANDLE_NIL == sending_datareader_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datareader handle");
      return false;
    }
    if (DDS::HANDLE_NIL == receiving_datawriter_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datawriter handle");
      return false;
    }

    // For the stub, just supply the input as the output
    ::DDS::OctetSeq transformed_buffer(encoded_rtps_message);
    plain_rtps_message.swap(transformed_buffer);

    return true;
  }

::CORBA::Boolean CryptoTransformBuiltInImpl::decode_serialized_payload(
  ::DDS::OctetSeq & plain_buffer,
  const ::DDS::OctetSeq & encoded_buffer,
  const ::DDS::OctetSeq & inline_qos,
  ::DDS::Security::DatareaderCryptoHandle receiving_datareader_crypto,
  ::DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
  ::DDS::Security::SecurityException & ex)
  {
    ACE_UNUSED_ARG(inline_qos);

    if (DDS::HANDLE_NIL == receiving_datareader_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datareader handle");
      return false;
    }
    if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
      CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datawriter handle");
      return false;
    }

    // For the stub, just supply the input as the output
    ::DDS::OctetSeq transformed_buffer(encoded_buffer);
    plain_buffer.swap(transformed_buffer);

    return true;
  }

} // Security
} // OpenDDS
