/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "LocalCredentialData.h"
#include "dds/DCPS/security/SSL/Utils.h"
#include <cstdio>
#include <cstring>
#include <cerrno>

namespace OpenDDS {
  namespace Security {

    static void add_binary_property(DDS::BinaryProperty_t src, DDS::BinaryPropertySeq& dst)
    {
      size_t len = dst.length();
      dst.length(len + 1);
      dst[len] = src;
    }

    int CredentialHash::operator()(DDS::OctetSeq& dst) const
    {
      const DDS::OctetSeq& perm_data = permissions_data_;
      const DDS::OctetSeq& topic_data = participant_topic_data_;

      DDS::BinaryPropertySeq hash_data;

      DDS::BinaryProperty_t cid, cperm, cpdata, cdsign_algo, ckagree_algo;

      cid.name = "c.id";
      cid.value = pubcert_.original_bytes();
      cid.propagate = true;

      cperm.name = "c.perm";
      cperm.value = perm_data;
      cperm.propagate = true;

      cpdata.name = "c.pdata";
      cpdata.value = topic_data;
      cpdata.propagate = true;

      cdsign_algo.name = "c.dsign_algo";
      const char* cdsign_algo_str = pubcert_.dsign_algo();
      cdsign_algo.value.length(std::strlen(cdsign_algo_str) + 1);
      std::memcpy(cdsign_algo.value.get_buffer(), cdsign_algo_str, cdsign_algo.value.length());
      cdsign_algo.propagate = true;

      ckagree_algo.name = "c.kagree_algo";
      const char* ckagree_algo_str = dh_.kagree_algo();
      ckagree_algo.value.length(std::strlen(ckagree_algo_str) + 1);
      std::memcpy(ckagree_algo.value.get_buffer(), ckagree_algo_str, ckagree_algo.value.length());
      ckagree_algo.propagate = true;

      add_binary_property(cid, hash_data);
      add_binary_property(cperm, hash_data);
      add_binary_property(cpdata, hash_data);
      add_binary_property(cdsign_algo, hash_data);
      add_binary_property(ckagree_algo, hash_data);

      return SSL::hash_serialized(hash_data, dst);
    }

    LocalAuthCredentialData::LocalAuthCredentialData(const DDS::PropertySeq& props)
    {
      load(props);
    }

    LocalAuthCredentialData::LocalAuthCredentialData()
    {

    }

    LocalAuthCredentialData::~LocalAuthCredentialData()
    {

    }

    void LocalAuthCredentialData::load(const DDS::PropertySeq& props)
    {
      std::string name, value, pkey_uri, password;
      for (size_t i = 0; i < props.length(); ++i) {
        name = props[i].name;
        value = props[i].value;

        if (name == "dds.sec.auth.identity_ca") {
            ca_cert_.reset(new SSL::Certificate(value));

        } else if (name == "dds.sec.auth.private_key") {
            pkey_uri = value;

        } else if (name == "dds.sec.auth.identity_certificate") {
            participant_cert_.reset(new SSL::Certificate(value));

        } else if (name == "dds.sec.auth.password") {
            password = value;

        } else if (name == "dds.sec.access.permissions") {

          std::string path;
          SSL::URI_SCHEME s = SSL::extract_uri_info(value, path);

          switch(s) {
            case SSL::URI_FILE:
              load_permissions_file(path);
              break;

            case SSL::URI_DATA:
            case SSL::URI_PKCS11:
            case SSL::URI_UNKNOWN:
            default:
              fprintf(stderr, "LocalAuthCredentialData::load: Error, unsupported URI scheme in path '%s'\n", value.c_str());
              break;
          }
        }
      }

      if (pkey_uri != "") {
        participant_pkey_.reset(new SSL::PrivateKey(pkey_uri, password));
      }
    }

    void LocalAuthCredentialData::load_permissions_file(const std::string& path)
    {
      std::vector<unsigned char> chunks;
      unsigned char chunk[32] = {0};

      FILE* fp = fopen(path.c_str(), "r");
      if (fp) {
        size_t count = 0u;
        while((count = fread(chunk, sizeof(chunk[0]), sizeof(chunk), fp))) {
          chunks.insert(chunks.end(),
                        chunk,
                        chunk + count);
        }

        if (ferror(fp)) {
            fprintf(stderr,
                    "LocalAuthCredentialData::load_permissions_file: Error '%s' occured while reading file '%s'\n",
                    std::strerror(errno),
                    path.c_str());
        } else {
            access_permissions_.length(chunks.size());
            std::memcpy(access_permissions_.get_buffer(), &chunks[0], access_permissions_.length());
        }

        // To appease the other DDS security implementations which
        // append a null byte at the end of the cert.
        access_permissions_.length(access_permissions_.length() + 1);
        access_permissions_[access_permissions_.length() - 1] = 0;

        fclose(fp);
      }
    }
  }
}
