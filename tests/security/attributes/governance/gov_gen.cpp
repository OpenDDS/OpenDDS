/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <iostream>
#include <fstream>
#include <string>

/* Basic Types */

// Boolean
const size_t bool_options_count = 2;
const std::string bool_option_names[bool_options_count] = { "FALSE", "TRUE" };

// Basic Protection Kind
const size_t basic_protection_kind_options_count = 3;
const std::string basic_protection_kind_option_names[basic_protection_kind_options_count] = { "NONE", "SIGN", "ENCRYPT" };

// Protection Kind
const size_t protection_kind_options_count = 5;
const std::string protection_kind_option_names[protection_kind_options_count] = { "NONE", "SIGN", "ENCRYPT", "SIGN_WITH_ORIGIN_AUTHENTICATION", "ENCRYPT_WITH_ORIGIN_AUTHENTICATION" };

/* Global Governance Flags */

// Allow Unauthenticated Participants (AUP)
const size_t global_aup_options_count = bool_options_count;
const std::string global_aup_options_names[global_aup_options_count] = bool_option_names;
const std::string global_aup_options_abbreviations[global_aup_options_count] = { "PU", "AU" };

// Enable Join Access Control (EJAC)
const size_t global_ejac_options_count = bool_options_count;
const std::string global_ejac_options_names[global_ejac_options_count] = bool_option_names;
const std::string global_ejac_options_abbreviations[global_ejac_options_count] = { "UA", "PA" };

// Discovery Protection Kind (DPK)
const size_t global_dpk_options_count = protection_kind_options_count;
const std::string global_dpk_options_names[global_dpk_options_count] = protection_kind_option_names;
const std::string global_dpk_options_abbreviations[global_dpk_options_count] = { "ND", "SD", "ED", "SOD", "EOD" };

// Liveliness Protection Kind (LPK)
const size_t global_lpk_options_count = protection_kind_options_count;
const std::string global_lpk_options_names[global_lpk_options_count] = protection_kind_option_names;
const std::string global_lpk_options_abbreviations[global_lpk_options_count] = { "NL", "SL", "EL", "SOL", "EOL" };

// RTPS Protection Kind (RPK)
const size_t global_rpk_options_count = protection_kind_options_count;
const std::string global_rpk_options_names[global_rpk_options_count] = protection_kind_option_names;
const std::string global_rpk_options_abbreviations[global_rpk_options_count] = { "NR", "SR", "ER", "SOR", "EOR" };

/* Topic Governance Flags */

// Enable Discovery Protection (EDP)
const size_t topic_edp_options_count = bool_options_count;
const std::string topic_edp_options_names[topic_edp_options_count] = bool_option_names;
const std::string topic_edp_options_abbreviations[topic_edp_options_count] = { "OD", "PD" };

// Enable Liveliness Protection (ELP)
const size_t topic_elp_options_count = bool_options_count;
const std::string topic_elp_options_names[topic_elp_options_count] = bool_option_names;
const std::string topic_elp_options_abbreviations[topic_elp_options_count] = { "OL", "PL" };

// Enable Read Access Control (ERAC)
const size_t topic_erac_options_count = bool_options_count;
const std::string topic_erac_options_names[topic_erac_options_count] = bool_option_names;
const std::string topic_erac_options_abbreviations[topic_erac_options_count] = { "OR", "PR" };

// Enable Write Access Control (EWAC)
const size_t topic_ewac_options_count = bool_options_count;
const std::string topic_ewac_options_names[topic_ewac_options_count] = bool_option_names;
const std::string topic_ewac_options_abbreviations[topic_ewac_options_count] = { "OW", "PW" };

// Combined Read / Write Access Control Abbreviations
const size_t topic_crwac_options_count = topic_erac_options_count * topic_ewac_options_count;
const std::string topic_crwac_options_abbreviations[topic_crwac_options_count] = { "OA", "RA", "WA", "RWA" };

// Metadata Protection Kind (MPK)
const size_t topic_mpk_options_count = protection_kind_options_count;
const std::string topic_mpk_options_names[topic_mpk_options_count] = protection_kind_option_names;
const std::string topic_mpk_options_abbreviations[topic_mpk_options_count] = { "OM", "SM", "EM", "SOM", "EOM" };

// Data Protection Kind (DPK)
const size_t topic_dpk_options_count = basic_protection_kind_options_count;
const std::string topic_dpk_options_names[topic_dpk_options_count] = basic_protection_kind_option_names;
const std::string topic_dpk_options_abbreviations[topic_dpk_options_count] = { "OD", "SD", "ED" };

const std::string single_indent = "  ";

const std::string prefix =
R"raw_prefix(<?xml version="1.0" encoding="UTF-8"?>
<dds xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://www.omg.org/spec/DDS-SECURITY/20160303/omg_shared_ca_governance.xsd">
  <domain_access_rules>
    <domain_rule>
      <domains>
        <id>0</id>
      </domains>
)raw_prefix";

const std::string global_key =
R"raw_global_key(      <!-- Naming for Governance files:
        AU / PU                  : Allow Unauthenticated / Prohibit Unauthenticated
        UA / PA                  : Unprotected Access / Protected Access
        ND / SD / ED / SOD / EOD : Not protected / Signed / Encrypted / Signed with Origin authentication / Encrypted with Origin authentication Discovery
        NL / SL / EL / SOL / EOL : Not protected / Signed / Encrypted / Signed with Origin authentication / Encrypted with Origin authentication Liveliness
        NR / SR / ER / SOR / EOR : Not protected / Signed / Encrypted / Signed with Origin authentication / Encrypted with Origin authentication RTPS
      -->
)raw_global_key";

const std::string topic_key =
R"raw_topic_key(      <!-- Naming for Topics:
        OD / PD                  : Open / Protected Discovery
        OL / PL                  : Open / Protected Liveliness
        OA / RA / WA / RWA       : Open / Read / Write / ReadWrite Access
        OM / SM / EM / SOM / EOM : Open / Signed / Encrypted / Signed with Origin authentication / Encrypted with Origin authentication Meta-data
        OD / SD / ED             : Open / Signed / Encrypted Data
      -->
)raw_topic_key";


const std::string suffix =
R"raw_suffix(    </domain_rule>
  </domain_access_rules>
</dds>
)raw_suffix";

const std::string filename_suffix = "_temporary";
const std::string filename_extension = ".xml";

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

void write_topic_rule_elements(std::ostream& os, size_t ilvl, const std::string& expression, size_t edp, size_t elp, size_t erac, size_t ewac, size_t mpk, size_t dpk)
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

void write_docs()
{

  for (size_t aup = 0; aup < global_aup_options_count; ++aup) {
    for (size_t ejac = 0; ejac < global_ejac_options_count; ++ejac) {
      for (size_t dpk = 0; dpk < global_dpk_options_count; ++dpk) {
        for (size_t lpk = 0; lpk < global_lpk_options_count; ++lpk) {
          for (size_t rpk = 0; rpk < global_rpk_options_count; ++rpk) {

            std::string global_name = generate_global_name(aup, ejac, dpk, lpk, rpk);
            std::string filename = std::string("governance_") + global_name + filename_suffix + filename_extension;

            std::cerr<< filename << std::endl;
            std::ofstream ofs(filename.data());

            write_full_doc(ofs, aup, ejac, dpk, lpk, rpk);

          }
        }
      }
    }
  }

}

int main(int argc, char** argv)
{
  write_docs();
  return 0;
}
