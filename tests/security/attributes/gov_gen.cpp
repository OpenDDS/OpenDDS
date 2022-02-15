/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <iostream>
#include <sstream>
#include <string>

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>

#include <openssl/pkcs7.h>
#include <openssl/pem.h>

size_t find(const std::string& option, const std::string& needle, const std::string arr[], size_t arr_size)
{
  for (size_t idx = 0; idx != arr_size; ++idx) {
    if (arr[idx] == needle) {
      return idx;
    }
  }

  std::cerr << option << " must not be one of";
  for (size_t idx = 0; idx != arr_size; ++idx) {
    std::cerr << " " << arr[idx];
  }
  std::cerr << std::endl;

  exit(EXIT_FAILURE);
  return -1;
}

/* Global Governance Flags */

// Allow Unauthenticated Participants (AUP)
const size_t global_aup_options_count = 2;
const std::string global_aup_options_names[global_aup_options_count] = { "FALSE", "TRUE" };
const std::string global_aup_options_abbreviations[global_aup_options_count] = { "PU", "AU" };

// Enable Join Access Control (EJAC)
const size_t global_ejac_options_count = 2;
const std::string global_ejac_options_names[global_ejac_options_count] = { "FALSE", "TRUE" };
const std::string global_ejac_options_abbreviations[global_ejac_options_count] = { "UA", "PA" };

// Discovery Protection Kind (DPK)
const size_t global_dpk_options_count = 5;
const std::string global_dpk_options_names[global_dpk_options_count] = { "NONE", "SIGN", "ENCRYPT", "SIGN_WITH_ORIGIN_AUTHENTICATION", "ENCRYPT_WITH_ORIGIN_AUTHENTICATION" };
const std::string global_dpk_options_abbreviations[global_dpk_options_count] = { "ND", "SD", "ED", "SOD", "EOD" };

// Liveliness Protection Kind (LPK)
const size_t global_lpk_options_count = 5;
const std::string global_lpk_options_names[global_lpk_options_count] = { "NONE", "SIGN", "ENCRYPT", "SIGN_WITH_ORIGIN_AUTHENTICATION", "ENCRYPT_WITH_ORIGIN_AUTHENTICATION" };
const std::string global_lpk_options_abbreviations[global_lpk_options_count] = { "NL", "SL", "EL", "SOL", "EOL" };

// RTPS Protection Kind (RPK)
const size_t global_rpk_options_count = 5;
const std::string global_rpk_options_names[global_rpk_options_count] = { "NONE", "SIGN", "ENCRYPT", "SIGN_WITH_ORIGIN_AUTHENTICATION", "ENCRYPT_WITH_ORIGIN_AUTHENTICATION" };
const std::string global_rpk_options_abbreviations[global_rpk_options_count] = { "NR", "SR", "ER", "SOR", "EOR" };

/* Topic Governance Flags */

// Enable Discovery Protection (EDP)
const size_t topic_edp_options_count = 2;
const std::string topic_edp_options_names[topic_edp_options_count] = { "FALSE", "TRUE" };
const std::string topic_edp_options_abbreviations[topic_edp_options_count] = { "OD", "PD" };

// Enable Liveliness Protection (ELP)
const size_t topic_elp_options_count = 2;
const std::string topic_elp_options_names[topic_elp_options_count] = { "FALSE", "TRUE" };
const std::string topic_elp_options_abbreviations[topic_elp_options_count] = { "OL", "PL" };

// Enable Read Access Control (ERAC)
const size_t topic_erac_options_count = 2;
const std::string topic_erac_options_names[topic_erac_options_count] = { "FALSE", "TRUE" };
const std::string topic_erac_options_abbreviations[topic_erac_options_count] = { "OR", "PR" };

// Enable Write Access Control (EWAC)
const size_t topic_ewac_options_count = 2;
const std::string topic_ewac_options_names[topic_ewac_options_count] = { "FALSE", "TRUE" };
const std::string topic_ewac_options_abbreviations[topic_ewac_options_count] = { "OW", "PW" };

// Combined Read / Write Access Control Abbreviations
const size_t topic_crwac_options_count = topic_erac_options_count * topic_ewac_options_count;
const std::string topic_crwac_options_abbreviations[topic_crwac_options_count] = { "OA", "RA", "WA", "RWA" };

// Metadata Protection Kind (MPK)
const size_t topic_mpk_options_count = 5;
const std::string topic_mpk_options_names[topic_mpk_options_count] = { "NONE", "SIGN", "ENCRYPT", "SIGN_WITH_ORIGIN_AUTHENTICATION", "ENCRYPT_WITH_ORIGIN_AUTHENTICATION" };
const std::string topic_mpk_options_abbreviations[topic_mpk_options_count] = { "OM", "SM", "EM", "SOM", "EOM" };

// Data Protection Kind (DPK)
const size_t topic_dpk_options_count = 3;
const std::string topic_dpk_options_names[topic_dpk_options_count] = { "NONE", "SIGN", "ENCRYPT" };
const std::string topic_dpk_options_abbreviations[topic_dpk_options_count] = { "OD", "SD", "ED" };

const std::string single_indent = "  ";

const std::string prefix =
               "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               "<dds xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.omg.org/spec/DDS-SECURITY/20170901/omg_shared_ca_governance.xsd\">\n"
               "  <domain_access_rules>\n"
               "    <domain_rule>\n"
               "      <domains>\n"
               "        <id>0</id>\n"
               "      </domains>\n";

const std::string global_key =
               "      <!-- Naming for Governance files:\n"
               "        AU / PU                  : Allow Unauthenticated / Prohibit Unauthenticated\n"
               "        UA / PA                  : Unprotected Access / Protected Access\n"
               "        ND / SD / ED / SOD / EOD : Not protected / Signed / Encrypted / Signed with Origin authentication / Encrypted with Origin authentication Discovery\n"
               "        NL / SL / EL / SOL / EOL : Not protected / Signed / Encrypted / Signed with Origin authentication / Encrypted with Origin authentication Liveliness\n"
               "        NR / SR / ER / SOR / EOR : Not protected / Signed / Encrypted / Signed with Origin authentication / Encrypted with Origin authentication RTPS\n"
               "      -->\n";

const std::string topic_key =
               "      <!-- Naming for Topics:\n"
               "        OD / PD                  : Open / Protected Discovery\n"
               "        OL / PL                  : Open / Protected Liveliness\n"
               "        OA / RA / WA / RWA       : Open / Read / Write / ReadWrite Access\n"
               "        OM / SM / EM / SOM / EOM : Open / Signed / Encrypted / Signed with Origin authentication / Encrypted with Origin authentication Meta-data\n"
               "        OD / SD / ED             : Open / Signed / Encrypted Data\n"
               "      -->\n";

const std::string suffix =
               "    </domain_rule>\n"
               "  </domain_access_rules>\n"
               "</dds>\n";

void write_indent(std::ostream& os, size_t ilvl)
{
  while (ilvl != 0)
    {
      os << single_indent;
      --ilvl;
    }
}

void write_element_line(std::ostream& os, size_t ilvl, const std::string& name, const std::string& value)
{
  write_indent(os, ilvl);
  os << "<" << name << ">" << value << "</" << name << ">" << std::endl;
}

void write_global_elements(std::ostream& os, size_t ilvl, size_t aup, size_t ejac, size_t dpk, size_t lpk, size_t rpk)
{
  write_element_line(os, ilvl, "allow_unauthenticated_participants", global_aup_options_names[aup]);
  write_element_line(os, ilvl, "enable_join_access_control", global_ejac_options_names[ejac]);
  write_element_line(os, ilvl, "discovery_protection_kind", global_dpk_options_names[dpk]);
  write_element_line(os, ilvl, "liveliness_protection_kind", global_lpk_options_names[lpk]);
  write_element_line(os, ilvl, "rtps_protection_kind", global_rpk_options_names[rpk]);
}

void write_topic_rule_elements(std::ostream& os, size_t ilvl, const std::string& expression, size_t edp, size_t /*elp*/, size_t erac, size_t ewac, size_t mpk, size_t dpk)
{
  write_element_line(os, ilvl, "topic_expression", expression);
  write_element_line(os, ilvl, "enable_discovery_protection", topic_edp_options_names[edp]);
  write_element_line(os, ilvl, "enable_liveliness_protection", topic_elp_options_names[edp]);
  write_element_line(os, ilvl, "enable_read_access_control", topic_erac_options_names[erac]);
  write_element_line(os, ilvl, "enable_write_access_control", topic_ewac_options_names[ewac]);
  write_element_line(os, ilvl, "metadata_protection_kind", topic_mpk_options_names[mpk]);
  write_element_line(os, ilvl, "data_protection_kind", topic_dpk_options_names[dpk]);
}

std::string generate_topic_name(size_t edp, size_t elp, size_t erac, size_t ewac, size_t mpk, size_t dpk)
{
  size_t crwac = (ewac << 1) + erac;

  std::string result("");
  result += topic_edp_options_abbreviations[edp];
  result += "_";
  result += topic_elp_options_abbreviations[elp];
  result += "_";
  result += topic_crwac_options_abbreviations[crwac];
  result += "_";
  result += topic_mpk_options_abbreviations[mpk];
  result += "_";
  result += topic_dpk_options_abbreviations[dpk];

  return result;
}

std::string generate_topic_set_name(size_t edp, size_t elp, size_t erac, size_t ewac, size_t mpk, size_t dpk)
{
  return std::string("SET_") + generate_topic_name(edp, elp, erac, ewac, mpk, dpk) + "*";
}

void write_topic_rules(std::ostream& os, size_t ilvl)
{
  write_indent(os, ilvl);
  os << "<topic_access_rules>" << std::endl;

  write_indent(os, ilvl + 1);
  os << "<!-- Individual Topic Rules -->" << std::endl;

  for (size_t edp = 0; edp < topic_edp_options_count; ++edp) {
    for (size_t elp = 0; elp < topic_elp_options_count; ++elp) {
      for (size_t erac = 0; erac < topic_erac_options_count; ++erac) {
        for (size_t ewac = 0; ewac < topic_ewac_options_count; ++ewac) {
          for (size_t mpk = 0; mpk < topic_mpk_options_count; ++mpk) {
            for (size_t dpk = 0; dpk < topic_dpk_options_count; ++dpk) {

              write_indent(os, ilvl + 1);
              os << "<topic_rule>" << std::endl;

              const std::string name = generate_topic_name(edp, elp, erac, ewac, mpk, dpk);
              write_topic_rule_elements(os, ilvl + 2, name, edp, elp, erac, ewac, mpk, dpk);

              write_indent(os, ilvl + 1);
              os << "</topic_rule>" << std::endl;

            }
          }
        }
      }
    }
  }

  write_indent(os, ilvl + 1);
  os << "<!-- Expression Rules -->" << std::endl;

  for (size_t edp = 0; edp < topic_edp_options_count; ++edp) {
    for (size_t elp = 0; elp < topic_elp_options_count; ++elp) {
      for (size_t erac = 0; erac < topic_erac_options_count; ++erac) {
        for (size_t ewac = 0; ewac < topic_ewac_options_count; ++ewac) {
          for (size_t mpk = 0; mpk < topic_mpk_options_count; ++mpk) {
            for (size_t dpk = 0; dpk < topic_dpk_options_count; ++dpk) {

              write_indent(os, ilvl + 1);
              os << "<topic_rule>" << std::endl;

              const std::string name = generate_topic_set_name(edp, elp, erac, ewac, mpk, dpk);
              write_topic_rule_elements(os, ilvl + 2, name, edp, elp, erac, ewac, mpk, dpk);

              write_indent(os, ilvl + 1);
              os << "</topic_rule>" << std::endl;

            }
          }
        }
      }
    }
  }

  write_indent(os, ilvl);
  os << "</topic_access_rules>" << std::endl;
}

void write_full_doc(std::ostream& os, size_t aup, size_t ejac, size_t dpk, size_t lpk, size_t rpk)
{
  os << prefix << std::flush;
  write_global_elements(os, 3, aup, ejac, dpk, lpk, rpk);
  os << topic_key << std::flush;
  write_topic_rules(os, 3);
  os << suffix << std::flush;
}

std::string generate_global_name(size_t aup, size_t ejac, size_t dpk, size_t lpk, size_t rpk)
{
  std::string result("");
  result += global_aup_options_abbreviations[aup];
  result += "_";
  result += global_ejac_options_abbreviations[ejac];
  result += "_";
  result += global_dpk_options_abbreviations[dpk];
  result += "_";
  result += global_lpk_options_abbreviations[lpk];
  result += "_";
  result += global_rpk_options_abbreviations[rpk];

  return result;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  std::string aup;
  std::string ejac;
  std::string dpk;
  std::string lpk;
  std::string rpk;
  std::string outpath;
  std::string certpath;
  std::string keypath;

  int argc_copy = argc;
  ACE_Argv_Type_Converter atc(argc_copy, argv);
  ACE_Arg_Shifter_T<char> shifter(atc.get_argc(), atc.get_ASCII_argv());
  shifter.ignore_arg(); // argv[0] is the program name
  while (shifter.is_anything_left()) {
    const char* arg = 0;
    if ((arg = shifter.get_the_parameter("-aup"))) {
      shifter.consume_arg();
      aup = arg;
    } else if ((arg = shifter.get_the_parameter("-ejac"))) {
      shifter.consume_arg();
      ejac = arg;
    } else if ((arg = shifter.get_the_parameter("-dpk"))) {
      shifter.consume_arg();
      dpk = arg;
    } else if ((arg = shifter.get_the_parameter("-lpk"))) {
      shifter.consume_arg();
      lpk = arg;
    } else if ((arg = shifter.get_the_parameter("-rpk"))) {
      shifter.consume_arg();
      rpk = arg;
    } else if ((arg = shifter.get_the_parameter("-o"))) {
      shifter.consume_arg();
      outpath = arg;
    } else if ((arg = shifter.get_the_parameter("-cert"))) {
      shifter.consume_arg();
      certpath = arg;
    } else if ((arg = shifter.get_the_parameter("-key"))) {
      shifter.consume_arg();
      keypath = arg;
    } else if (shifter.get_current() == std::string("-help")) {
      std::cout << "gov_gen -aup [PU|AU] -ejac [UA|PA] -dpk [ND|SD|ED|SOD|EOD] -lpk [NL|SL|EL|SOL|EOL] -rpk [NR|SR|ER|SOR|EOR] -o OUTPUT -cert CERT -key KEY" << std::endl;
      return EXIT_SUCCESS;
    } else {
      std::cerr << "ERROR: invalid option: " << shifter.get_current() << " (try -help)" << std::endl;
      return EXIT_FAILURE;
    }
  }

  const size_t aup_idx = find("-aup", aup, global_aup_options_abbreviations, global_aup_options_count);
  const size_t ejac_idx = find("-ejac", ejac, global_ejac_options_abbreviations, global_ejac_options_count);
  const size_t dpk_idx = find("-dpk", dpk, global_dpk_options_abbreviations, global_dpk_options_count);
  const size_t lpk_idx = find("-lpk", lpk, global_lpk_options_abbreviations, global_lpk_options_count);
  const size_t rpk_idx = find("-rpk", rpk, global_rpk_options_abbreviations, global_rpk_options_count);

  if (outpath.empty()) {
    std::cerr << "ERROR: -o may not be empty" << std::endl;
    return EXIT_FAILURE;
  }

  if (certpath.empty()) {
    std::cerr << "ERROR: -cert may not be empty" << std::endl;
    return EXIT_FAILURE;
  }

  if (keypath.empty()) {
    std::cerr << "ERROR: -key may not be empty" << std::endl;
    return EXIT_FAILURE;
  }

  std::stringstream buffer;
  write_full_doc(buffer, aup_idx, ejac_idx, dpk_idx, lpk_idx, rpk_idx);

  X509* cert;
  EVP_PKEY* key;

  {
    BIO* filebuf = BIO_new_file(certpath.c_str(), "r");
    if (!filebuf) {
      std::cerr << "ERROR: could not open " << certpath << std::endl;
      return EXIT_FAILURE;
    }

    cert = PEM_read_bio_X509(filebuf, NULL, 0, NULL);
    if (!cert) {
      std::cerr << "ERROR: could not read " << certpath << std::endl;
      return EXIT_FAILURE;
    }

    BIO_free(filebuf);
  }

  {
    BIO* filebuf = BIO_new_file(keypath.c_str(), "r");
    if (!filebuf) {
      std::cerr << "ERROR: could not open " << keypath << std::endl;
      return EXIT_FAILURE;
    }

    key = PEM_read_bio_PrivateKey(filebuf, NULL, NULL, NULL);
    if (!key) {
      std::cerr << "ERROR: could not key " << keypath << std::endl;
      return EXIT_FAILURE;
    }

    BIO_free(filebuf);
  }

  BIO* mem = BIO_new(BIO_s_mem());
  if (BIO_write(mem, buffer.str().data(), buffer.str().size()) != static_cast<ssize_t>(buffer.str().size())) {
    std::cerr << "ERROR: could not copy to BIO" << std::endl;
    return EXIT_FAILURE;
  }

  PKCS7* p7 = PKCS7_sign(cert, key, NULL, NULL, PKCS7_TEXT | PKCS7_DETACHED);
  if (!p7) {
    std::cerr << "ERROR: could not sign" << std::endl;
    return EXIT_FAILURE;
  }

  BIO* out = BIO_new_file(outpath.c_str(), "w");
  if (out == NULL) {
    std::cerr << "ERROR: could not open " << outpath << std::endl;
    return EXIT_FAILURE;

  }

  if (!SMIME_write_PKCS7(out, p7, mem, PKCS7_TEXT | PKCS7_DETACHED)) {
    std::cerr << "ERROR: could not write " << outpath << std::endl;
    return EXIT_FAILURE;
  }

  BIO_free(out);
  PKCS7_free(p7);
  BIO_free(mem);
  EVP_PKEY_free(key);
  X509_free(cert);

  return EXIT_SUCCESS;
}
