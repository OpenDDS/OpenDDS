/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "Permissions.h"

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

Permissions::Permissions()
  : perm_data_(), subject_name_()
{
}

bool Permissions::extract_subject_name(const SSL::SignedDocument& doc)
{
  doc.get_original_minus_smime(subject_name_);

  const std::string start_str("<subject_name>"), end_str("</subject_name>");

  const size_t found_begin = subject_name_.find(start_str);

  if (found_begin != std::string::npos) {
    subject_name_.erase(0, found_begin + start_str.length());
    const char* t = " \t\n\r\f\v";
    subject_name_.erase(0, subject_name_.find_first_not_of(t));

  } else {
    return false;
  }

  const size_t found_end = subject_name_.find(end_str);

  if (found_end != std::string::npos) {
    subject_name_.erase(found_end);
  } else {
    return false;
  }

  return true;
}

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

  if (!extract_subject_name(doc)) {
    ACE_ERROR((LM_ERROR, "Permissions::load: WARNING, Could not extract subject name from permissions file"));
    return -1;
  }

  DCPS::unique_ptr<xercesc::XercesDOMParser> parser(new xercesc::XercesDOMParser());
  parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);    // optional
  parser->setCreateCommentNodes(false);

  DCPS::unique_ptr<xercesc::ErrorHandler> errHandler((xercesc::ErrorHandler*) new xercesc::HandlerBase());
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
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Exception message is %C.\n"), message));
    xercesc::XMLString::release(&message);
    return -1;

  } catch (const xercesc::DOMException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.msg);
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Exception message is: %C.\n"), message));
    xercesc::XMLString::release(&message);
    return -1;

  } catch (...) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Unexpected Permissions XML Parser Exception.\n")));
    return -1;
  }


  // Successfully parsed the permissions file

  xercesc::DOMDocument* xmlDoc = parser->getDocument();

  xercesc::DOMElement* elementRoot = xmlDoc->getDocumentElement();
  if (!elementRoot) {
    throw std::runtime_error("empty XML document");
  }

  //TODO:  WARNING - this implementation only supports 1 permissions/grant set
  // Different from governance from here forward
  // Find the validity rules
  xercesc::DOMNodeList * grantRules = xmlDoc->getElementsByTagName(XStr("grant"));

  for (XMLSize_t r = 0; r < grantRules->getLength(); ++r) {
    PermissionGrantRule rule_holder_;

    // Pull out the grant name for this grant
    xercesc::DOMNamedNodeMap * rattrs = grantRules->item(r)->getAttributes();
    rule_holder_.grant_name = toString(rattrs->item(0)->getTextContent());

    // Pull out subject name, validity, and default
    xercesc::DOMNodeList * grantNodes = grantRules->item(r)->getChildNodes();

    for (XMLSize_t gn = 0; gn < grantNodes->getLength(); gn++) {
      const XStr g_tag = grantNodes->item(gn)->getNodeName();

      if (g_tag == "subject_name") {
        rule_holder_.subject = toString(grantNodes->item(gn)->getTextContent());
      } else if (g_tag == "validity") {
        //Validity_t gn_validity;
        xercesc::DOMNodeList *validityNodes = grantNodes->item(gn)->getChildNodes();

        for (XMLSize_t vn = 0; vn < validityNodes->getLength(); vn++) {
          const XStr v_tag = validityNodes->item(vn)->getNodeName();

          if (v_tag == "not_before") {
            rule_holder_.validity.not_before = toString(
                      (validityNodes->item(vn)->getTextContent()));
          } else if (v_tag == "not_after") {
            rule_holder_.validity.not_after = toString(
                      (validityNodes->item(vn)->getTextContent()));
          }
        }
      } else if (g_tag == "default") {
        rule_holder_.default_permission = toString(grantNodes->item(gn)->getTextContent());
      }
    }
    // Pull out allow/deny rules
    xercesc::DOMNodeList * adGrantNodes = grantRules->item(r)->getChildNodes();

    for (XMLSize_t gn = 0; gn < adGrantNodes->getLength(); gn++) {
      const XStr g_tag = adGrantNodes->item(gn)->getNodeName();

      if (g_tag == "allow_rule" || g_tag == "deny_rule") {
        PermissionTopicRule ptr_holder_;
        PermissionsPartition pp_holder_;

        ptr_holder_.ad_type = (g_tag == "allow_rule") ? ALLOW : DENY;
        pp_holder_.ad_type = (g_tag == "allow_rule") ? ALLOW : DENY;

        xercesc::DOMNodeList * adNodeChildren = adGrantNodes->item(gn)->getChildNodes();

        for (XMLSize_t anc = 0; anc < adNodeChildren->getLength(); anc++) {
          const XStr anc_tag = adNodeChildren->item(anc)->getNodeName();

          if (anc_tag == "domains") {   //domain list
            xercesc::DOMNodeList * domainIdNodes = adNodeChildren->item(anc)->getChildNodes();

            for (XMLSize_t did = 0; did < domainIdNodes->getLength(); did++) {
              if ("id" == XStr(domainIdNodes->item(did)->getNodeName())) {
                ptr_holder_.domain_list.insert(toInt(domainIdNodes->item(did)->getTextContent()));
                pp_holder_.domain_list.insert(toInt(domainIdNodes->item(did)->getTextContent()));
              } else if ("id_range" == XStr(domainIdNodes->item(did)->getNodeName())) {
                int min_value = 0;
                int max_value = 0;
                xercesc::DOMNodeList * domRangeIdNodes = domainIdNodes->item(did)->getChildNodes();

                for (XMLSize_t drid = 0; drid < domRangeIdNodes->getLength(); drid++) {
                  if ("min" == XStr(domRangeIdNodes->item(drid)->getNodeName())) {
                    min_value = toInt(domRangeIdNodes->item(drid)->getTextContent());
                  } else if ("max" == XStr(domRangeIdNodes->item(drid)->getNodeName())) {
                    max_value = toInt(domRangeIdNodes->item(drid)->getTextContent());

                    if ((min_value == 0) || (min_value > max_value)) {
                      ACE_DEBUG((LM_ERROR, ACE_TEXT(
                          "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Permission XML Domain Range invalid.\n")));
                      return -1;
                    }

                    for (int i = min_value; i <= max_value; i++) {
                      ptr_holder_.domain_list.insert(i);
                      pp_holder_.domain_list.insert(i);
                    }
                  }
                }
              }
            }

          } else if (anc_tag == "publish" || anc_tag == "subscribe") {   // pub sub nodes
            PermissionTopicPsRule anc_ps_rule_holder_;
            PermissionPartitionPs anc_ps_partition_holder_;

            anc_ps_rule_holder_.ps_type = (anc_tag == "publish") ? PUBLISH : SUBSCRIBE;
            anc_ps_partition_holder_.ps_type = anc_ps_rule_holder_.ps_type;
            xercesc::DOMNodeList * topicListNodes = adNodeChildren->item(anc)->getChildNodes();

            for (XMLSize_t tln = 0; tln < topicListNodes->getLength(); tln++) {
              if ("topics" == XStr(topicListNodes->item(tln)->getNodeName())) {
                xercesc::DOMNodeList * topicNodes = topicListNodes->item(tln)->getChildNodes();

                for (XMLSize_t tn = 0; tn < topicNodes->getLength(); tn++) {
                  if ("topic" == XStr(topicNodes->item(tn)->getNodeName())) {
                    anc_ps_rule_holder_.topic_list.push_back(toString(topicNodes->item(tn)->getTextContent()));
                  }
                }

              } else if ("partitions" == XStr(topicListNodes->item(tln)->getNodeName())) {
                xercesc::DOMNodeList * partitionNodes = topicListNodes->item(tln)->getChildNodes();

                for (XMLSize_t pn = 0; pn < partitionNodes->getLength(); pn++) {
                  if ("partition" == XStr(partitionNodes->item(pn)->getNodeName())) {
                    anc_ps_partition_holder_.partition_list.push_back(toString(partitionNodes->item(pn)->getTextContent()));
                  }
                }
              }
            }

            ptr_holder_.topic_ps_rules.push_back(anc_ps_rule_holder_);
            pp_holder_.partition_ps.push_back(anc_ps_partition_holder_);
          }
        }

        rule_holder_.PermissionTopicRules.push_back(ptr_holder_);
        rule_holder_.PermissionPartitions.push_back(pp_holder_);
      }
    }

    perm_data_.perm_rules.push_back(rule_holder_);
  } // grant_rules

  return 0;
}


}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
