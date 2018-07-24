/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
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

    SignedDocument(const SignedDocument& rhs);

    SignedDocument();

    virtual ~SignedDocument();

    SignedDocument& operator=(const SignedDocument& rhs);

    void load(const std::string& uri);

    void get_original(std::string& dst) const;

    const DDS::OctetSeq& get_original() const
    {
      return original_;
    }

    const std::string& get_verifiable() const
    {
      return verifiable_;
    }

    bool get_content_minus_smime(std::string& cleaned_content) const;

    /**
     * @return int 0 on success; 1 on failure.
     */
    int verify_signature(const Certificate& ca) const;

    /**
     * @return int 0 on success; 1 on failure.
     */
    int serialize(DDS::OctetSeq& dst) const;

    /**
     * @return int 0 on success; 1 on failure.
     */
    int deserialize(const DDS::OctetSeq& src);

    /**
     * @return int 0 on success; 1 on failure.
     */
    int deserialize(const std::string& src);

   private:

    /**
     * @return int 0 on success; 1 on failure.
     */
    int cache_verifiable();

    PKCS7* PKCS7_from_SMIME_file(const std::string& path);

    PKCS7* PKCS7_from_data(const DDS::OctetSeq& s_mime_data);

    PKCS7* doc_;
    BIO* content_;
    DDS::OctetSeq original_;
    std::string verifiable_;
  };

  DdsSecurity_Export bool operator==(const SignedDocument& lhs,
                                     const SignedDocument& rhs);

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

#endif
