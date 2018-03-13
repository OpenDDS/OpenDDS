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

    int CredentialHash::operator()(DDS::OctetSeq& dst) const
    {
      const DDS::OctetSeq& cperm = permissions_data_;
      const DDS::OctetSeq& cpdata = participant_topic_data_;

      DDS::OctetSeq cid;
      pubcert_.serialize(cid);

      DDS::OctetSeq cdsign_algo;
      std::string cdsign_algo_str = pubcert_.dsign_algo();
      cdsign_algo.length(cdsign_algo_str.length());
      std::memcpy(cdsign_algo.get_buffer(), cdsign_algo_str.c_str(), cdsign_algo.length());

      DDS::OctetSeq ckagree_algo;
      std::string ckagree_algo_str = dh_.kagree_algo();
      ckagree_algo.length(ckagree_algo_str.length());
      std::memcpy(ckagree_algo.get_buffer(), ckagree_algo_str.c_str(), ckagree_algo.length());

      std::vector<const DDS::OctetSeq*> data;
      data.push_back(&cid);
      data.push_back(&cperm);
      data.push_back(&cpdata);
      data.push_back(&cdsign_algo);
      data.push_back(&ckagree_algo);

      return SSL::hash(data, dst);
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

      participant_pkey_.reset(new SSL::PrivateKey(pkey_uri, password));
    }

    void LocalAuthCredentialData::load_permissions_file(const std::string& path)
    {
      std::vector<unsigned char> chunks;
      unsigned char chunk[32] = {0};

      FILE* fp = fopen(path.c_str(), "r");
      if (fp) {
        size_t bytes = 0u;
        while((bytes = fread(chunk, sizeof(chunk), 1, fp) > 0)) {
          chunks.insert(chunks.end(), chunk, chunk + bytes);
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

        fclose(fp);
      }
    }
  }
}
