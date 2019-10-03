/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_FRAMEWORK_PROPERTIES_H
#define OPENDDS_DCPS_SECURITY_FRAMEWORK_PROPERTIES_H

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {
namespace Security {
namespace Properties {

const char AccessGovernance[] =        "dds.sec.access.governance";
const char AccessPermissions[] =       "dds.sec.access.permissions";
const char AccessPermissionsCA[] =     "dds.sec.access.permissions_ca";
const char AuthIdentityCA[] =          "dds.sec.auth.identity_ca";
const char AuthIdentityCertificate[] = "dds.sec.auth.identity_certificate";
const char AuthPassword[] =            "dds.sec.auth.password";
const char AuthPrivateKey[] =          "dds.sec.auth.private_key";

}
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_SECURITY_FRAMEWORK_PROPERTIES_H */
