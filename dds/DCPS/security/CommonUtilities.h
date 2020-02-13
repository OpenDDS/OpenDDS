#ifndef DDS_SECURITY_UTILS_H
#define DDS_SECURITY_UTILS_H

#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"

#include "dds/DCPS/PoolAllocator.h"
#include "CryptoBuiltInC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace CommonUtilities {


/// @brief This URI abstraction is currently naive and only separates the URI scheme
/// on the LHS from the "everything-else" of the URI on the RHS. As such this may only handle
/// the URI_FILE and URI_DATA cases properly. Further investigate into URI_PKCS11
/// should be completed.
struct URI {
  enum Scheme
  {
    URI_UNKNOWN,
    URI_FILE,
    URI_DATA,
    URI_PKCS11,
  };

  explicit URI(const std::string& src);

  Scheme scheme;
  std::string everything_else;
};

int increment_handle(int& next);

bool set_security_error(DDS::Security::SecurityException& ex,
                        int code,
                        int minor_code,
                        const char* message);

bool set_security_error(DDS::Security::SecurityException& ex,
                        int code,
                        int minor_code,
                        const char* message,
                        const unsigned char (&a1)[4],
                        const unsigned char (&a2)[4]);

const char* ctk_to_dds_string(const CryptoTransformKind& keyKind);
OPENDDS_STRING ctki_to_dds_string(const CryptoTransformKeyId& keyId);
OPENDDS_STRING to_dds_string(const KeyOctetSeq& keyData);
OPENDDS_STRING to_dds_string(const KeyMaterial_AES_GCM_GMAC& km);
OPENDDS_STRING to_dds_string(const CryptoTransformIdentifier& id);

}
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
