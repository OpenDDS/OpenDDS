/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "Permissions.h"

#include "dds/DCPS/security/AccessControlBuiltInImpl.h"

#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/framework/MemBufInputSource.hpp"

#include "ace/OS_NS_strings.h"
#include "ace/XML_Utils/XercesString.h"

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

namespace {
  std::string toString(const XMLCh* in)
  {
    char* c = xercesc::XMLString::transcode(in);
    const std::string s(c);
    xercesc::XMLString::release(&c);
    return s;
  }

  int toInt(const XMLCh* in)
  {
    unsigned int i = 0;
    xercesc::XMLString::textToBin(in, i);
    return static_cast<int>(i);
  }
}

int Permissions::load(const SSL::SignedDocument& doc)
{
  using XML::XStr;
  static const char* gMemBufId = "gov buffer id";

  DCPS::unique_ptr<xercesc::XercesDOMParser> parser(new xercesc::XercesDOMParser());
  parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);    // optional
  parser->setCreateCommentNodes(false);

  DCPS::unique_ptr<xercesc::ErrorHandler> errHandler(new xercesc::HandlerBase());
  parser->setErrorHandler(errHandler.get());

  std::string cleaned;
  doc.get_original_minus_smime(cleaned);
  xercesc::MemBufInputSource contentbuf((const XMLByte*) cleaned.c_str(),
                                        cleaned.size(),
                                        gMemBufId);
  try {
    parser->parse(contentbuf);

  } catch (const xercesc::XMLException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.getMessage());
    ACE_ERROR((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Exception message is %C.\n"), message));
    xercesc::XMLString::release(&message);
    return -1;

  } catch (const xercesc::DOMException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.msg);
    ACE_ERROR((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Exception message is: %C.\n"), message));
    xercesc::XMLString::release(&message);
    return -1;

  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Unexpected Permissions XML Parser Exception.\n")));
    return -1;
  }


  // Successfully parsed the permissions file

  xercesc::DOMDocument* xmlDoc = parser->getDocument();

  xercesc::DOMElement* elementRoot = xmlDoc->getDocumentElement();
  if (!elementRoot) {
    ACE_ERROR((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Empty XML document\n")));
    return -1;
  }

  // Find the validity rules
  xercesc::DOMNodeList* grantRules = xmlDoc->getElementsByTagName(XStr(ACE_TEXT("grant")));

  for (XMLSize_t r = 0; r < grantRules->getLength(); ++r) {
    Grant_rch grant = DCPS::make_rch<Grant>();

    // Pull out the grant name for this grant
    xercesc::DOMNamedNodeMap* rattrs = grantRules->item(r)->getAttributes();
    grant->name = toString(rattrs->item(0)->getTextContent());

    // Pull out subject name, validity, and default
    xercesc::DOMNodeList* grantNodes = grantRules->item(r)->getChildNodes();

    bool valid_subject = false, valid_default = false;
    for (XMLSize_t gn = 0; gn < grantNodes->getLength(); ++gn) {
      const XStr g_tag = grantNodes->item(gn)->getNodeName();

      if (g_tag == ACE_TEXT("subject_name")) {
        valid_subject = (grant->subject.parse(toString(grantNodes->item(gn)->getTextContent())) == 0);
      } else if (g_tag == ACE_TEXT("validity")) {
        xercesc::DOMNodeList* validityNodes = grantNodes->item(gn)->getChildNodes();

        for (XMLSize_t vn = 0; vn < validityNodes->getLength(); ++vn) {
          const XStr v_tag = validityNodes->item(vn)->getNodeName();

          if (v_tag == ACE_TEXT("not_before")) {
            grant->validity.not_before = toString((validityNodes->item(vn)->getTextContent()));
          } else if (v_tag == ACE_TEXT("not_after")) {
            grant->validity.not_after = toString((validityNodes->item(vn)->getTextContent()));
          }
        }
      } else if (g_tag == ACE_TEXT("default")) {
        const std::string def = toString(grantNodes->item(gn)->getTextContent());
        valid_default = true;
        if (def == "ALLOW") {
          grant->default_permission = ALLOW;
        } else if (def == "DENY") {
          grant->default_permission = DENY;
        } else {
          ACE_ERROR((LM_ERROR, ACE_TEXT(
            "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: <default> must be ALLOW or DENY\n")));
          return -1;
        }
      }
    }

    if (!valid_default) {
      ACE_ERROR((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: <default> is required\n")));
      return -1;
    }

    // Pull out allow/deny rules
    xercesc::DOMNodeList* adGrantNodes = grantRules->item(r)->getChildNodes();

    for (XMLSize_t gn = 0; gn < adGrantNodes->getLength(); ++gn) {
      const XStr g_tag = adGrantNodes->item(gn)->getNodeName();

      if (g_tag == ACE_TEXT("allow_rule") || g_tag == ACE_TEXT("deny_rule")) {
        Rule rule;

        rule.ad_type = (g_tag == ACE_TEXT("allow_rule")) ? ALLOW : DENY;

        xercesc::DOMNodeList* adNodeChildren = adGrantNodes->item(gn)->getChildNodes();

        for (XMLSize_t anc = 0; anc < adNodeChildren->getLength(); ++anc) {
          const XStr anc_tag = adNodeChildren->item(anc)->getNodeName();

          if (anc_tag == ACE_TEXT("domains")) {   //domain list
            xercesc::DOMNodeList* domainIdNodes = adNodeChildren->item(anc)->getChildNodes();

            for (XMLSize_t did = 0; did < domainIdNodes->getLength(); ++did) {
              if (ACE_TEXT("id") == XStr(domainIdNodes->item(did)->getNodeName())) {
                rule.domains.insert(toInt(domainIdNodes->item(did)->getTextContent()));
              } else if (ACE_TEXT("id_range") == XStr(domainIdNodes->item(did)->getNodeName())) {
                int min_value = 0;
                int max_value = 0;
                xercesc::DOMNodeList* domRangeIdNodes = domainIdNodes->item(did)->getChildNodes();

                for (XMLSize_t drid = 0; drid < domRangeIdNodes->getLength(); ++drid) {
                  if (ACE_TEXT("min") == XStr(domRangeIdNodes->item(drid)->getNodeName())) {
                    min_value = toInt(domRangeIdNodes->item(drid)->getTextContent());
                  } else if (ACE_TEXT("max") == XStr(domRangeIdNodes->item(drid)->getNodeName())) {
                    max_value = toInt(domRangeIdNodes->item(drid)->getTextContent());

                    if (min_value > max_value) {
                      ACE_ERROR((LM_ERROR, ACE_TEXT(
                          "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Permission XML Domain Range invalid.\n")));
                      return -1;
                    }

                    for (int i = min_value; i <= max_value; ++i) {
                      rule.domains.insert(i);
                    }
                  }
                }
              }
            }

          } else if (anc_tag == ACE_TEXT("publish") || anc_tag == ACE_TEXT("subscribe")) {   // pub sub nodes
            Action action;

            action.ps_type = (anc_tag == ACE_TEXT("publish")) ? PUBLISH : SUBSCRIBE;
            xercesc::DOMNodeList* topicListNodes = adNodeChildren->item(anc)->getChildNodes();

            for (XMLSize_t tln = 0; tln < topicListNodes->getLength(); ++tln) {
              if (ACE_TEXT("topics") == XStr(topicListNodes->item(tln)->getNodeName())) {
                xercesc::DOMNodeList* topicNodes = topicListNodes->item(tln)->getChildNodes();

                for (XMLSize_t tn = 0; tn < topicNodes->getLength(); ++tn) {
                  if (ACE_TEXT("topic") == XStr(topicNodes->item(tn)->getNodeName())) {
                    action.topics.push_back(toString(topicNodes->item(tn)->getTextContent()));
                  }
                }

              } else if (ACE_TEXT("partitions") == XStr(topicListNodes->item(tln)->getNodeName())) {
                xercesc::DOMNodeList* partitionNodes = topicListNodes->item(tln)->getChildNodes();

                for (XMLSize_t pn = 0; pn < partitionNodes->getLength(); ++pn) {
                  if (ACE_TEXT("partition") == XStr(partitionNodes->item(pn)->getNodeName())) {
                    action.partitions.push_back(toString(partitionNodes->item(pn)->getTextContent()));
                  }
                }
              }
            }

            rule.actions.push_back(action);
          }
        }

        grant->rules.push_back(rule);
      }
    }

    if (!valid_subject) {
      if (DCPS::security_debug.access_warn) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {access_warn} ")
          ACE_TEXT("AccessControlBuiltInImpl::load_permissions_file: ")
          ACE_TEXT("Unable to parse subject name, ignoring grant.\n")));
      }
    } else if (find_grant(grant->subject)) {
      if (DCPS::security_debug.access_warn) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {access_warn} ")
          ACE_TEXT("AccessControlBuiltInImpl::load_permissions_file: ")
          ACE_TEXT("Ignoring grant with duplicate subject name.\n")));
      }
    } else {
      grants_.push_back(grant);
    }
  } // grant_rules

  return 0;
}

bool Permissions::find_grant(const SSL::SubjectName& name, Grant_rch* found) const
{
  for (Grants::const_iterator it = grants_.begin(); it != grants_.end(); ++it) {
    if (name == (*it)->subject) {
      if (found) {
        *found = *it;
      }
      return true;
    }
  }
  return false;
}

namespace {
  typedef std::vector<std::string>::const_iterator vsiter_t;
}

bool Permissions::Action::topic_matches(const char* topic) const
{
  for (vsiter_t it = topics.begin(); it != topics.end(); ++it) {
    if (AccessControlBuiltInImpl::pattern_match(topic, it->c_str())) {
      return true;
    }
  }
  return false;
}

bool Permissions::Action::partitions_match(const DDS::StringSeq& entity_partitions, AllowDeny_t allow_or_deny) const
{
  const unsigned int n_entity_names = entity_partitions.length();
  if (partitions.empty()) {
    if (allow_or_deny == DENY) {
      // DDS-Security v1.1 9.4.1.3.2.3.2.4
      // If there is no <partitions> section ... the deny action would
      // apply independent of the partition associated with the DDS Endpoint
      return true;
    }
    // DDS-Security v1.1 9.4.1.3.2.3.1.4
    // If there is no <partitions> Section within an allow rule, then the default "empty string" partition is
    // assumed. ... This means that the allow rule would only allow a DataWriter to publish on
    // the "empty string" partition.
    // DDS v1.4 2.2.3 "PARTITION"
    // The zero-length sequence is treated as a special value equivalent to a sequence containing a single
    // element consisting of the empty string.
    return n_entity_names == 0 || (n_entity_names == 1 && entity_partitions[0].in()[0] == 0);
  }

  for (unsigned int i = 0; i < n_entity_names; ++i) {
    bool found = false;
    for (vsiter_t perm_it = partitions.begin(); !found && perm_it != partitions.end(); ++perm_it) {
      if (AccessControlBuiltInImpl::pattern_match(entity_partitions[i], perm_it->c_str())) {
        found = true;
      }
    }
    if (allow_or_deny == ALLOW && !found) {
      // DDS-Security v1.1 9.4.1.3.2.3.1.4
      // In order for an action to meet the allowed partitions condition that appears
      // within an allow rule, the set of the Partitions associated with the DDS entity
      // ... must be contained in the set of partitions defined by the allowed partitions
      // condition section.
      return false; // i'th QoS partition name is not matched by any <partition> in Permissions
    }
    if (allow_or_deny == DENY && found) {
      // DDS-Security v1.1 9.4.1.3.2.3.2.4
      // In order for an action to be denied it must meet the denied partitions condition.
      // For this to happen one [or] more of the partition names associated with the DDS Entity
      // ... must match one [of] the partitions ... listed in the partitions condition section.
      return true; // i'th QoS partition name matches some <partition> in Permissons
    }
  }

  return allow_or_deny == ALLOW;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
