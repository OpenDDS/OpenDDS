/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "Governance.h"

#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/util/PlatformUtils.hpp"

#include "ace/OS_NS_strings.h"
#include "ace/XML_Utils/XercesString.h"

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

Governance::TopicAccessRule::TopicAccessRule()
{
  topic_attrs.is_read_protected = false;
  topic_attrs.is_write_protected = false;
  topic_attrs.is_discovery_protected = false;
  topic_attrs.is_liveliness_protected = false;
}

Governance::Governance()
{
}

int Governance::load(const SSL::SignedDocument& doc)
{
  using XML::XStr;
  static const char* gMemBufId = "gov buffer id";

  xercesc::XMLPlatformUtils::Initialize();
  DCPS::unique_ptr<xercesc::XercesDOMParser> parser(new xercesc::XercesDOMParser());

  if (!parser) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
      "(%P|%t) AccessControlBuiltInImpl::load_governance_file: Governance XML DOMParser Exception.\n")));
    return -1;
  }

  parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);    // optional
  parser->setCreateCommentNodes(false);

  DCPS::unique_ptr<xercesc::ErrorHandler> errHandler((xercesc::ErrorHandler*) new xercesc::HandlerBase());
  parser->setErrorHandler(errHandler.get());

  // buffer for parsing

  std::string cleaned;
  doc.get_original_minus_smime(cleaned);

  xercesc::MemBufInputSource contentbuf((const XMLByte*) cleaned.c_str(),
                                        cleaned.size(),
                                        gMemBufId);

  try {
    parser->parse(contentbuf);

  } catch (const xercesc::XMLException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.getMessage());
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_governance_file: Exception message is %C.\n"), message));
    xercesc::XMLString::release(&message);
    return -1;

  } catch (const xercesc::DOMException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.msg);
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_governance_file: Exception message is %C.\n"), message));
    xercesc::XMLString::release(&message);
    return -1;

  } catch (...) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_governance_file: Unexpected Governance XML Parser Exception.\n")));
    return -1;
  }

  // Successfully parsed the governance file

  const xercesc::DOMDocument* xmlDoc = parser->getDocument();

  const xercesc::DOMElement* elementRoot = xmlDoc->getDocumentElement();
  if (!elementRoot) {
    throw std::runtime_error("empty XML document");
  }

  // Find the domain rules
  const xercesc::DOMNodeList* domainRules = xmlDoc->getElementsByTagName(XStr(ACE_TEXT("domain_rule")));

  for (XMLSize_t r = 0, dr_len = domainRules->getLength(); r < dr_len; ++r) {
    Governance::DomainRule rule_holder_;
    rule_holder_.domain_attrs.plugin_participant_attributes = 0;

    const xercesc::DOMNode* domainRule = domainRules->item(r);

    // Pull out domain ids used in the rule. We are NOT supporting ranges at this time
    const xercesc::DOMNodeList* ruleNodes = domainRule->getChildNodes();

    for (XMLSize_t rn = 0, rn_len = ruleNodes->getLength(); rn < rn_len; rn++) {

      const xercesc::DOMNode* ruleNode = ruleNodes->item(rn);

      const XStr dn_tag = ruleNode->getNodeName();

      if (ACE_TEXT("domains") == dn_tag) {
        const xercesc::DOMNodeList* domainIdNodes = ruleNode->getChildNodes();

        for (XMLSize_t did = 0, did_len = domainIdNodes->getLength(); did < did_len; did++) {

          const xercesc::DOMNode* domainIdNode = domainIdNodes->item(did);

          if (ACE_TEXT("id") == XStr(domainIdNode->getNodeName())) {
            const XMLCh* t = domainIdNode->getTextContent();
            unsigned int i;
            if (xercesc::XMLString::textToBin(t, i)) {
              rule_holder_.domain_list.insert(i);
            }
          } else if (ACE_TEXT("id_range") == XStr(domainIdNode->getNodeName())) {
            int min_value = 0;
            int max_value = 0;

            const xercesc::DOMNodeList* domRangeIdNodes = domainIdNode->getChildNodes();

            for (XMLSize_t drid = 0, drid_len = domRangeIdNodes->getLength(); drid < drid_len; drid++) {

              const xercesc::DOMNode* domRangeIdNode = domRangeIdNodes->item(drid);

              if (ACE_OS::strcmp("min", xercesc::XMLString::transcode(domRangeIdNode->getNodeName())) == 0) {
                min_value = atoi(xercesc::XMLString::transcode(domRangeIdNode->getTextContent()));
              } else if (strcmp("max", xercesc::XMLString::transcode(domRangeIdNode->getNodeName())) == 0) {
                max_value = atoi(xercesc::XMLString::transcode(domRangeIdNode->getTextContent()));

                if (min_value > max_value) {
                  ACE_DEBUG((LM_ERROR, ACE_TEXT(
                      "(%P|%t) AccessControlBuiltInImpl::load_governance_file: Governance XML Domain Range invalid.\n")));
                  return -1;
                }

                for (int i = min_value; i <= max_value; ++i) {
                  rule_holder_.domain_list.insert(i);
                }
              }
            }
          }
        }

      }
    }

    // Process allow_unauthenticated_participants
    const xercesc::DOMNodeList* allow_unauthenticated_participants_ =
              xmlDoc->getElementsByTagName(XStr(ACE_TEXT("allow_unauthenticated_participants")));
    char* attr_aup = xercesc::XMLString::transcode(allow_unauthenticated_participants_->item(0)->getTextContent());
    rule_holder_.domain_attrs.allow_unauthenticated_participants = (ACE_OS::strcasecmp(attr_aup,"false") == 0 ? false : true);
    xercesc::XMLString::release(&attr_aup);

    // Process enable_join_access_control
    const xercesc::DOMNodeList* enable_join_access_control_ =
             xmlDoc->getElementsByTagName(XStr(ACE_TEXT("enable_join_access_control")));
    char* attr_ejac = xercesc::XMLString::transcode(enable_join_access_control_->item(0)->getTextContent());
    rule_holder_.domain_attrs.is_access_protected = ACE_OS::strcasecmp(attr_ejac, "false") == 0 ? false : true;
    xercesc::XMLString::release(&attr_ejac);

    // Process discovery_protection_kind
    const xercesc::DOMNodeList* discovery_protection_kind_ =
             xmlDoc->getElementsByTagName(XStr(ACE_TEXT("discovery_protection_kind")));
    char* attr_dpk = xercesc::XMLString::transcode(discovery_protection_kind_->item(0)->getTextContent());
    if (ACE_OS::strcasecmp(attr_dpk, "NONE") == 0) {
      rule_holder_.domain_attrs.is_discovery_protected = false;
    } else if (ACE_OS::strcasecmp(attr_dpk, "SIGN") == 0) {
      rule_holder_.domain_attrs.is_discovery_protected = true;
    } else if (ACE_OS::strcasecmp(attr_dpk, "ENCRYPT") == 0) {
      rule_holder_.domain_attrs.is_discovery_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED;
    } else if (ACE_OS::strcasecmp(attr_dpk, "SIGN_WITH_ORIGIN_AUTHENTICATION") == 0) {
      rule_holder_.domain_attrs.is_discovery_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED;
    } else if (ACE_OS::strcasecmp(attr_dpk, "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") == 0) {
      rule_holder_.domain_attrs.is_discovery_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED;
    }
    xercesc::XMLString::release(&attr_dpk);

    // Process liveliness_protection_kind
    const xercesc::DOMNodeList* liveliness_protection_kind_ =
             xmlDoc->getElementsByTagName(XStr(ACE_TEXT("liveliness_protection_kind")));
    char* attr_lpk = xercesc::XMLString::transcode(liveliness_protection_kind_->item(0)->getTextContent());
    if (ACE_OS::strcasecmp(attr_lpk, "NONE") == 0) {
      rule_holder_.domain_attrs.is_liveliness_protected = false;
    } else if (ACE_OS::strcasecmp(attr_lpk, "SIGN") == 0) {
      rule_holder_.domain_attrs.is_liveliness_protected = true;
    } else if (ACE_OS::strcasecmp(attr_lpk, "ENCRYPT") == 0) {
      rule_holder_.domain_attrs.is_liveliness_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED;
    } else if (ACE_OS::strcasecmp(attr_lpk, "SIGN_WITH_ORIGIN_AUTHENTICATION") == 0) {
      rule_holder_.domain_attrs.is_liveliness_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED;
    } else if (ACE_OS::strcasecmp(attr_lpk, "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") == 0) {
      rule_holder_.domain_attrs.is_liveliness_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED;
    }
    xercesc::XMLString::release(&attr_lpk);

    // Process rtps_protection_kind
    const xercesc::DOMNodeList* rtps_protection_kind_ =
             xmlDoc->getElementsByTagName(XStr(ACE_TEXT("rtps_protection_kind")));
    char* attr_rpk = xercesc::XMLString::transcode(rtps_protection_kind_->item(0)->getTextContent());

    if (ACE_OS::strcasecmp(attr_rpk, "NONE") == 0) {
      rule_holder_.domain_attrs.is_rtps_protected = false;
    } else if (ACE_OS::strcasecmp(attr_rpk, "SIGN") == 0) {
      rule_holder_.domain_attrs.is_rtps_protected = true;
    } else if (ACE_OS::strcasecmp(attr_rpk, "ENCRYPT") == 0) {
      rule_holder_.domain_attrs.is_rtps_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED;
    } else if (ACE_OS::strcasecmp(attr_rpk, "SIGN_WITH_ORIGIN_AUTHENTICATION") == 0) {
      rule_holder_.domain_attrs.is_rtps_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED;
    } else if (ACE_OS::strcasecmp(attr_rpk, "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") == 0) {
      rule_holder_.domain_attrs.is_rtps_protected = true;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED;
      rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED;
    }
    xercesc::XMLString::release(&attr_rpk);

    rule_holder_.domain_attrs.plugin_participant_attributes |= ::DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;

    // Process topic rules

    const xercesc::DOMNodeList* topic_rules = xmlDoc->getElementsByTagName(XStr(ACE_TEXT("topic_rule")));

    for (XMLSize_t tr = 0, tr_len = topic_rules->getLength(); tr < tr_len; tr++) {

      const xercesc::DOMNode* topic_rule = topic_rules->item(tr);

      const xercesc::DOMNodeList* topic_rule_nodes = topic_rule->getChildNodes();
      Governance::TopicAccessRule t_rules;

      for (XMLSize_t trn = 0, trn_len = topic_rule_nodes->getLength(); trn < trn_len; trn++) {

        const xercesc::DOMNode* topic_rule_node = topic_rule_nodes->item(trn);

        const XStr tr_tag = topic_rule_node->getNodeName();
        char* tr_val = xercesc::XMLString::transcode(topic_rule_node->getTextContent());

        if (tr_tag == ACE_TEXT("topic_expression")) {
          t_rules.topic_expression = tr_val;
        } else if (tr_tag == ACE_TEXT("enable_discovery_protection")) {
          t_rules.topic_attrs.is_discovery_protected = ACE_OS::strcasecmp(tr_val, "false") == 0 ? false : true;
        } else if (tr_tag == ACE_TEXT("enable_liveliness_protection")) {
          t_rules.topic_attrs.is_liveliness_protected = ACE_OS::strcasecmp(tr_val, "false") == 0 ? false : true;
        } else if (tr_tag == ACE_TEXT("enable_read_access_control")) {
          t_rules.topic_attrs.is_read_protected = ACE_OS::strcasecmp(tr_val, "false") == 0 ? false : true;
        } else if (tr_tag == ACE_TEXT("enable_write_access_control")) {
          t_rules.topic_attrs.is_write_protected = ACE_OS::strcasecmp(tr_val, "false") == 0 ? false : true;
        } else if (tr_tag == ACE_TEXT("metadata_protection_kind")) {
          t_rules.metadata_protection_kind.assign(tr_val);
        } else if (tr_tag == ACE_TEXT("data_protection_kind")) {
          t_rules.data_protection_kind.assign(tr_val);
        }
        xercesc::XMLString::release(&tr_val);
      }
      rule_holder_.topic_rules.push_back(t_rules);
    }

    access_rules_.push_back(rule_holder_);
  } // domain_rule

  return 0;
}


}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
