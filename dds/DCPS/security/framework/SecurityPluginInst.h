/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_FRAMEWORK_SECURITYPLUGININST_H
#define OPENDDS_DCPS_SECURITY_FRAMEWORK_SECURITYPLUGININST_H

#ifdef OPENDDS_SECURITY
#include "dds/DdsSecurityCoreC.h"
#include "dds/DCPS/security/Utility.h"
#endif

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject.h"

#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

#ifdef OPENDDS_SECURITY
using DDS::Security::Authentication_var;
using DDS::Security::AccessControl_var;
using DDS::Security::CryptoKeyFactory_var;
using DDS::Security::CryptoKeyExchange_var;
using DDS::Security::CryptoTransform_var;
#endif

/**
 * @class SecurityPluginInst
 *
 * @brief Base class for concrete security plugins to provide new objects.
 *
 * Each security plugin implementation will need to define a concrete
 * subclass of the SecurityPluginType class.  The base
 * class contains the pure virtual functions to
 * provide new objects. The concrete plugin implements these methods
 * to provide the implementations of the various plugins.
 *
 */
class OpenDDS_Dcps_Export SecurityPluginInst : public DCPS::RcObject {
public:

#ifdef OPENDDS_SECURITY
  // Factory methods for the plugin specific interfaces.  A SecurityPluginInst
  // may not support creating all of these interfaces
  virtual Authentication_var create_authentication() = 0;
  virtual AccessControl_var create_access_control() = 0;
  virtual CryptoKeyExchange_var create_crypto_key_exchange() = 0;
  virtual CryptoKeyFactory_var create_crypto_key_factory() = 0;
  virtual CryptoTransform_var create_crypto_transform() = 0;
  virtual DCPS::RcHandle<Utility> create_utility() = 0;
#endif

  // Perform any logic needed when shutting down the plugin
  virtual void shutdown() = 0;

protected:

  SecurityPluginInst();
  virtual ~SecurityPluginInst();
};


} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
