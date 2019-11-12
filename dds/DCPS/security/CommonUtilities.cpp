#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DCPS/SafetyProfileStreams.h"

#include <string>
#include <cstdio>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace CommonUtilities {

using OpenDDS::DCPS::to_hex_dds_string;

URI::URI(const std::string& src)
  : scheme(URI_UNKNOWN), everything_else("") //authority(), path(""), query(""), fragment("")
{
  typedef std::vector<std::pair<std::string, Scheme> > uri_pattern_t;

  uri_pattern_t uri_patterns;
  uri_patterns.push_back(std::make_pair("file:", URI_FILE));
  uri_patterns.push_back(std::make_pair("data:", URI_DATA));
  uri_patterns.push_back(std::make_pair("pkcs11:", URI_PKCS11));

  for (uri_pattern_t::iterator i = uri_patterns.begin();
       i != uri_patterns.end(); ++i) {
    const std::string& pfx = i->first;
    size_t pfx_end = pfx.length();

    if (src.substr(0, pfx_end) == pfx) {
      everything_else = src.substr(pfx_end, std::string::npos);
      scheme = i->second;
      break;
    }
  }
}

int increment_handle(int& next)
{
  // handles are 32-bit signed values (int on all supported platforms)
  // the only special value is 0 for HANDLE_NIL, 'next' starts at 1
  // signed increment is not guaranteed to roll over so we implement our own
  static const int LAST_POSITIVE_HANDLE(0x7fffffff);
  static const int FIRST_NEGATIVE_HANDLE(-LAST_POSITIVE_HANDLE);
  if (next == 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) OpenDDS::Security::CommonUtilities::"
               "increment_handle ERROR - out of handles\n"));
    return 0;
  }
  const int h = next;
  if (next == LAST_POSITIVE_HANDLE) {
    next = FIRST_NEGATIVE_HANDLE;
  } else {
    ++next;
  }
  return h;
}

bool set_security_error(DDS::Security::SecurityException& ex,
                        int code,
                        int minor_code,
                        const char* message)
{
  ex.code = code;
  ex.minor_code = minor_code;
  ex.message = message;
  return false;
}

bool set_security_error(DDS::Security::SecurityException& ex,
                        int code,
                        int minor_code,
                        const char* message,
                        const unsigned char (&a1)[4],
                        const unsigned char (&a2)[4])
{
  std::string full(message);
  const size_t i = full.size();
  full.resize(i + 25);
  std::sprintf(&full[i], " %.2x %.2x %.2x %.2x, %.2x %.2x %.2x %.2x",
               a1[0], a1[1], a1[2], a1[3], a2[0], a2[1], a2[2], a2[3]);
  return set_security_error(ex, code, minor_code, full.c_str());
}

const char* ctk_to_dds_string(const CryptoTransformKind& keyKind)
{
  if (!keyKind[0] && !keyKind[1] && !keyKind[2]) {
    switch (keyKind[3]) {
    case CRYPTO_TRANSFORMATION_KIND_NONE:
      return "CRYPTO_TRANSFORMATION_KIND_NONE";
    case CRYPTO_TRANSFORMATION_KIND_AES128_GMAC:
      return "CRYPTO_TRANSFORMATION_KIND_AES128_GMAC";
    case CRYPTO_TRANSFORMATION_KIND_AES128_GCM:
      return "CRYPTO_TRANSFORMATION_KIND_AES128_GCM";
    case CRYPTO_TRANSFORMATION_KIND_AES256_GMAC:
      return "CRYPTO_TRANSFORMATION_KIND_AES256_GMAC";
    case CRYPTO_TRANSFORMATION_KIND_AES256_GCM:
      return "CRYPTO_TRANSFORMATION_KIND_AES256_GCM";
    }
  }
  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Security::CommonUtilities::ctk_to_dds_string: ")
    ACE_TEXT("%C is either invalid or not recognized.\n"),
    to_hex_dds_string(keyKind, sizeof(keyKind), ' ').c_str()));
  return "Invalid CryptoTransformKind";
}

OPENDDS_STRING ctki_to_dds_string(const CryptoTransformKeyId& keyId)
{
  return to_hex_dds_string(keyId, sizeof(keyId), ' ');
}

OPENDDS_STRING to_dds_string(const KeyOctetSeq& keyData)
{
  if (keyData.length()) {
    return to_hex_dds_string(&keyData[0], keyData.length(), '\n', 8);
  }
  return "";
}

OPENDDS_STRING to_dds_string(const KeyMaterial_AES_GCM_GMAC& km)
{
  return
    OPENDDS_STRING("transformation_kind: ") +
    ctk_to_dds_string(km.transformation_kind) +
    OPENDDS_STRING("\nmaster_salt:\n") +
    to_dds_string(km.master_salt) +
    OPENDDS_STRING("\nsender_key_id: ") +
    ctki_to_dds_string(km.sender_key_id) +
    OPENDDS_STRING("\nmaster_sender_key:\n") +
    to_dds_string(km.master_sender_key) +
    OPENDDS_STRING("\nreceiver_specific_key_id: ") +
    ctki_to_dds_string(km.receiver_specific_key_id) +
    OPENDDS_STRING("\nmaster_receiver_specific_key:\n") +
    to_dds_string(km.master_receiver_specific_key) +
    OPENDDS_STRING("\n");
}

OPENDDS_STRING to_dds_string(const CryptoTransformIdentifier& id)
{
  return
    OPENDDS_STRING("transformation_kind: ") +
    ctk_to_dds_string(id.transformation_kind) +
    OPENDDS_STRING("\ntransformation_key_id: ") +
    ctki_to_dds_string(id.transformation_key_id) +
    OPENDDS_STRING("\n");
}

}
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
