/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "Governance.h"

#include "XmlUtils.h"

#include <dds/DCPS/debug.h>

#include <ace/OS_NS_strings.h>
#include <ace/XML_Utils/XercesString.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

using XML::XStr;
using namespace XmlUtils;
using namespace DDS::Security;
using OpenDDS::DCPS::security_debug;

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

namespace {
  bool get_bool_tag_value(const SSL::SignedDocument& doc, const xercesc::DOMNode* node,
    const ACE_TCHAR* name, bool& value)
  {
    if (!parse_bool(node, value)) {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} Governance::load: "
          "\"%s\" value, \"%C\", in \"%C\" is not a valid boolean value\n",
          name, to_string(node).c_str(), doc.filename().c_str()));
      }
      return false;
    }

    return true;
  }

  bool get_bool_tag(const SSL::SignedDocument& doc, const xercesc::DOMElement* parent,
    const ACE_TCHAR* name, bool& value)
  {
    const xercesc::DOMNodeList* const nodes = parent->getElementsByTagName(XStr(name));
    if (nodes->getLength() != 1) {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} Governance::load: "
          "expected 1 boolean value \"%s\" in parent element in \"%C\", found %B\n",
          name, doc.filename().c_str(), nodes->getLength()));
      }
      return false;
    }

    return get_bool_tag_value(doc, nodes->item(0), name, value);
  }

  bool get_protection_kind(const SSL::SignedDocument& doc, const xercesc::DOMElement* parent,
    const ACE_TCHAR* name, bool& is_protected, CORBA::ULong& attributes,
    CORBA::ULong enc_attr, CORBA::ULong oa_attr)
  {
    const xercesc::DOMNodeList* const nodes = parent->getElementsByTagName(XStr(name));
    if (nodes->getLength() != 1) {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} Governance::load: "
          "expected 1 proctection kind value named \"%s\" in parent element in \"%C\", found %B\n",
          name, doc.filename().c_str(), nodes->getLength()));
      }
      return false;
    }

    const xercesc::DOMNode* const node = nodes->item(0);
    const std::string value = to_string(node);
    if (ACE_OS::strcasecmp(value.c_str(), "NONE") == 0) {
      is_protected = false;
    } else if (ACE_OS::strcasecmp(value.c_str(), "SIGN") == 0) {
      is_protected = true;
    } else if (ACE_OS::strcasecmp(value.c_str(), "ENCRYPT") == 0) {
      is_protected = true;
      attributes |= enc_attr;
    } else if (ACE_OS::strcasecmp(value.c_str(), "SIGN_WITH_ORIGIN_AUTHENTICATION") == 0) {
      is_protected = true;
      attributes |= oa_attr;
    } else if (ACE_OS::strcasecmp(value.c_str(), "ENCRYPT_WITH_ORIGIN_AUTHENTICATION") == 0) {
      is_protected = true;
      attributes |= enc_attr;
      attributes |= oa_attr;
    } else {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} Governance::load: "
          "invalid %s, \"%C\", in \"%s\"\n",
          name, value.c_str(), doc.filename().c_str()));
      }
      return false;
    }

    return true;
  }
}

int Governance::load(const SSL::SignedDocument& doc)
{
  std::string xml;
  doc.get_original_minus_smime(xml);
  ParserPtr parser;
  if (!get_parser(parser, doc.filename(), xml)) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} Governance::load: "
        "get_parser failed\n"));
    }
    return -1;
  }

  // Find the domain rules
  const xercesc::DOMNodeList* const domainRules = parser->getDocument()->getDocumentElement()->
    getElementsByTagName(XStr(ACE_TEXT("domain_rule")));
  for (XMLSize_t r = 0, dr_len = domainRules->getLength(); r < dr_len; ++r) {
    Governance::DomainRule domain_rule;
    domain_rule.domain_attrs.plugin_participant_attributes = 0;
    const xercesc::DOMElement* const domain_rule_el =
      dynamic_cast<const xercesc::DOMElement*>(domainRules->item(r));
    if (!domain_rule_el) {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} Governance::load: "
          "domain_rule_el is null\n"));
      }
      return -1;
    }

    // Process domain ids this domain rule applies to
    const xercesc::DOMNodeList* const ruleNodes = domain_rule_el->getChildNodes();
    for (XMLSize_t rn = 0, rn_len = ruleNodes->getLength(); rn < rn_len; rn++) {
      const xercesc::DOMNode* const ruleNode = ruleNodes->item(rn);
      const XStr dn_tag = ruleNode->getNodeName();
      if (ACE_TEXT("domains") == dn_tag) {
        if (!parse_domain_id_set(ruleNode, domain_rule.domains)) {
          if (security_debug.access_error) {
            ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} Governance::load: "
              "failed to process domain ids in \"%C\"\n",
              doc.filename().c_str()));
          }
          return -1;
        }
      }
    }

    // Process allow_unauthenticated_participants
    if (!get_bool_tag(doc, domain_rule_el, ACE_TEXT("allow_unauthenticated_participants"),
        domain_rule.domain_attrs.allow_unauthenticated_participants)) {
      return -1;
    }

    // Process enable_join_access_control
    if (!get_bool_tag(doc, domain_rule_el, ACE_TEXT("enable_join_access_control"),
        domain_rule.domain_attrs.is_access_protected)) {
      return -1;
    }

    // Process discovery_protection_kind
    if (!get_protection_kind(doc, domain_rule_el, ACE_TEXT("discovery_protection_kind"),
        domain_rule.domain_attrs.is_discovery_protected,
        domain_rule.domain_attrs.plugin_participant_attributes,
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_BUILTIN_IS_DISCOVERY_ENCRYPTED,
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_ORIGIN_AUTHENTICATED)) {
      return -1;
    }

    // Process liveliness_protection_kind
    if (!get_protection_kind(doc, domain_rule_el, ACE_TEXT("liveliness_protection_kind"),
        domain_rule.domain_attrs.is_liveliness_protected,
        domain_rule.domain_attrs.plugin_participant_attributes,
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ENCRYPTED,
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_ORIGIN_AUTHENTICATED)) {
      return -1;
    }

    // Process rtps_protection_kind
    if (!get_protection_kind(doc, domain_rule_el, ACE_TEXT("rtps_protection_kind"),
        domain_rule.domain_attrs.is_rtps_protected,
        domain_rule.domain_attrs.plugin_participant_attributes,
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED,
        PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED)) {
      return -1;
    }

    domain_rule.domain_attrs.plugin_participant_attributes |=
      PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;

    // Process topic rules
    const xercesc::DOMNodeList* topic_rules =
      domain_rule_el->getElementsByTagName(XStr(ACE_TEXT("topic_rule")));
    for (XMLSize_t tr = 0, tr_len = topic_rules->getLength(); tr < tr_len; tr++) {
      const xercesc::DOMNode* topic_rule = topic_rules->item(tr);
      const xercesc::DOMNodeList* topic_rule_nodes = topic_rule->getChildNodes();
      Governance::TopicAccessRule t_rules;
      for (XMLSize_t trn = 0, trn_len = topic_rule_nodes->getLength(); trn < trn_len; trn++) {
        const xercesc::DOMNode* topic_rule_node = topic_rule_nodes->item(trn);
        const std::string name = to_string(topic_rule_node->getNodeName());

        bool* bool_value = 0;
        if (name == "topic_expression") {
          t_rules.topic_expression = to_string(topic_rule_node);
        } else if (name == "enable_discovery_protection") {
          bool_value = &t_rules.topic_attrs.is_discovery_protected;
        } else if (name == "enable_liveliness_protection") {
          bool_value = &t_rules.topic_attrs.is_liveliness_protected;
        } else if (name == "enable_read_access_control") {
          bool_value = &t_rules.topic_attrs.is_read_protected;
        } else if (name == "enable_write_access_control") {
          bool_value = &t_rules.topic_attrs.is_write_protected;
        } else if (name == "metadata_protection_kind") {
          t_rules.metadata_protection_kind = to_string(topic_rule_node);
        } else if (name == "data_protection_kind") {
          t_rules.data_protection_kind = to_string(topic_rule_node);
        }

        if (bool_value && !get_bool_tag_value(doc, topic_rule_node,
            ACE_TEXT_CHAR_TO_TCHAR(name.c_str()), *bool_value)) {
          return -1;
        }
      }
      domain_rule.topic_rules.push_back(t_rules);
    }

    access_rules_.push_back(domain_rule);
  } // domain_rule

  return 0;
}


}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
