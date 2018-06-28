/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_SignedDocument_H
#define OPENDDS_SECURITY_SSL_SignedDocument_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DCPS/unique_ptr.h"
#include "Certificate.h"
#include <string>
#include <openssl/pkcs7.h>

namespace OpenDDS {
namespace Security {
namespace SSL {

  class DdsSecurity_Export SignedDocument
  {
   public:
    typedef DCPS::unique_ptr<SignedDocument> unique_ptr;

    friend DdsSecurity_Export bool operator==(const SignedDocument& lhs,
                                              const SignedDocument& rhs);

    SignedDocument(const std::string& uri);

    SignedDocument(const DDS::OctetSeq& src);

    SignedDocument();

    virtual ~SignedDocument();

    SignedDocument& operator=(const SignedDocument& rhs);

    void load(const std::string& uri);

    void get_content(std::string& dst) const;

    int verify_signature(const Certificate& ca) const;

    int serialize(std::vector<unsigned char>& dst) const;

    int serialize(DDS::OctetSeq& dst) const;

    int deserialize(const DDS::OctetSeq& src);

   private:
    int cache_plaintext();

    PKCS7* PKCS7_from_SMIME(const std::string& path);

    PKCS7* PKCS7_from_DATA(const std::string& path);

    PKCS7* doc_;
    BIO* content_;
    std::string plaintext_;
  };

  DdsSecurity_Export bool operator==(const SignedDocument& lhs,
                                     const SignedDocument& rhs);

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

#endif
