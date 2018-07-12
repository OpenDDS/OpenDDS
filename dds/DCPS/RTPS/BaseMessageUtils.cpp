/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BaseMessageUtils.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

int locator_to_address(ACE_INET_Addr& dest,
                       const DCPS::Locator_t& locator,
                       bool map /*map IPV4 to IPV6 addr*/)
{
  switch (locator.kind) {
#ifdef ACE_HAS_IPV6
  case LOCATOR_KIND_UDPv6:
    dest.set_type(AF_INET6);
    if (dest.set_address(reinterpret_cast<const char*>(locator.address),
                         16, 0 /*encode*/) == -1) {
      return -1;
    }
    dest.set_port_number(locator.port);
    return 0;
#endif
  case LOCATOR_KIND_UDPv4:
#if !defined (ACE_HAS_IPV6) || !defined (IPV6_V6ONLY)
    ACE_UNUSED_ARG(map);
#endif
    dest.set_type(AF_INET);
    if (dest.set_address(reinterpret_cast<const char*>(locator.address)
                         + 12, 4, 0 /*network order*/
#if defined (ACE_HAS_IPV6) && defined (IPV6_V6ONLY)
                         , map ? 1 : 0 /*map IPV4 to IPV6 addr*/
#endif
                         ) == -1) {
      return -1;
    }
    dest.set_port_number(locator.port);
    return 0;
  default:
    return -1;  // Unknown kind
  }

  return -1;
}

DDS::ReturnCode_t blob_to_locators(const DCPS::TransportBLOB& blob,
                                   DCPS::LocatorSeq& locators,
                                   bool* requires_inline_qos,
                                   unsigned int* pBytesRead)
{
  ACE_Data_Block db(blob.length(), ACE_Message_Block::MB_DATA,
    reinterpret_cast<const char*>(blob.get_buffer()),
    0 /*alloc*/, 0 /*lock*/, ACE_Message_Block::DONT_DELETE, 0 /*db_alloc*/);
  ACE_Message_Block mb(&db, ACE_Message_Block::DONT_DELETE, 0 /*mb_alloc*/);
  mb.wr_ptr(mb.space());

  DCPS::Serializer ser(&mb, ACE_CDR_BYTE_ORDER, DCPS::Serializer::ALIGN_CDR);
  if (!(ser >> locators)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) blob_to_locators: ")
                      ACE_TEXT("Failed to deserialize blob's locators\n")),
                      DDS::RETCODE_ERROR);
  }

  if (requires_inline_qos) {
    if (!(ser >> ACE_InputCDR::to_boolean(*requires_inline_qos))) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) blob_to_locators: ")
                        ACE_TEXT("Failed to deserialize blob inline QoS flag\n")),
                       DDS::RETCODE_ERROR);
    }
  } else {
    if (!ser.skip(1)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) blob_to_locators: ")
                        ACE_TEXT("Failed to skip blob inline QoS flag\n")),
                       DDS::RETCODE_ERROR);
    }
  }

  if (pBytesRead) {
    *pBytesRead = blob.length() - mb.length();
  }
  return DDS::RETCODE_OK;
}

void locators_to_blob(const DCPS::LocatorSeq& locators,
                      DCPS::TransportBLOB& blob)
{
  using OpenDDS::DCPS::Serializer;
  size_t size_locator = 0, padding_locator = 0;
  DCPS::gen_find_size(locators, size_locator, padding_locator);
  ACE_Message_Block mb_locator(size_locator + padding_locator + 1);
  Serializer ser_loc(&mb_locator, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser_loc << locators;
  // Add a bool for 'requires inline qos', see Sedp::set_inline_qos():
  // if the bool is no longer the last octet of the sequence then that function
  // must be changed as well.
  ser_loc << ACE_OutputCDR::from_boolean(false);
  message_block_to_sequence(mb_locator, blob);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
