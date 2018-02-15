/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DDS_SECURITY_HELPERS_H
#define DDS_SECURITY_HELPERS_H

#include "dds/DdsSecurityCoreC.h"

namespace {

  inline DDS::Security::EndpointSecurityAttributesMask
    security_attribs_to_bitmask(const DDS::Security::EndpointSecurityAttributes& attribs)
  {
    using namespace DDS::Security;

    EndpointSecurityAttributesMask result = 0u;

    if (attribs.base.is_read_protected)
      result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_READ_PROTECTED;

    if (attribs.base.is_write_protected)
      result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_WRITE_PROTECTED;

    if (attribs.base.is_discovery_protected)
      result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;

    if (attribs.base.is_liveliness_protected)
      result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED;

    if (attribs.is_submessage_protected)
      result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED;

    if (attribs.is_payload_protected)
      result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED;

    if (attribs.is_key_protected)
      result |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_KEY_PROTECTED;

    return result;
  }

  inline void security_bitmask_to_attribs(const DDS::Security::EndpointSecurityAttributesMask& src,
                                         DDS::Security::EndpointSecurityAttributes& dest)
  {
    using namespace DDS::Security;

    dest.base.is_read_protected = (src & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_READ_PROTECTED);
    dest.base.is_write_protected = (src & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_WRITE_PROTECTED);
    dest.base.is_discovery_protected = (src & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED);
    dest.base.is_liveliness_protected = (src & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED);
    dest.is_submessage_protected = (src & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED);
    dest.is_payload_protected = (src & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED);
    dest.is_key_protected = (src & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_KEY_PROTECTED);
  }


}


#endif
