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
#include <stdexcept>

namespace OpenDDS {
namespace Security {

Permissions::Permissions()
  : perm_data_(), subject_name_()
{

}

bool Permissions::extract_subject_name(const SSL::SignedDocument& doc)
{
  doc.get_content_minus_smime(subject_name_);

  const std::string start_str("<subject_name>"), end_str("</subject_name>");

  size_t found_begin = subject_name_.find(start_str);

  if (found_begin != std::string::npos) {
    subject_name_.erase(0, found_begin + start_str.length());
    const char* t = " \t\n\r\f\v";
    subject_name_.erase(0, subject_name_.find_first_not_of(t));

  } else {
    return false;
  }

  size_t found_end = subject_name_.find(end_str);

  if (found_end != std::string::npos) {
    subject_name_.erase(found_end);
  } else {
    return false;
  }

  return true;
}

int Permissions::load(const SSL::SignedDocument& doc)
{
  static const char* gMemBufId = "gov buffer id";

  if (!extract_subject_name(doc))
  {
    ACE_ERROR((LM_ERROR, "Permissions::load: WARNING, Could not extract subject name from permissions file"));
    return -1;
  }

  try
  {
    xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
  }
  catch( xercesc::XMLException& e )
  {
    char* message = xercesc::XMLString::transcode( e.getMessage() );
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
      "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: XML toolkit initialization error:  %C.\n"), message));
    xercesc::XMLString::release( &message );
    // throw exception here to return ERROR_XERCES_INIT
    return -1;
  }

  xercesc::XercesDOMParser* parser = new xercesc::XercesDOMParser();
  parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);    // optional
  parser->setCreateCommentNodes(false);

  xercesc::ErrorHandler* errHandler = (xercesc::ErrorHandler*) new xercesc::HandlerBase();
  parser->setErrorHandler(errHandler);

  std::string cleaned;
  doc.get_content_minus_smime(cleaned);
  xercesc::MemBufInputSource contentbuf((const XMLByte*) cleaned.c_str(),
                                        cleaned.size(),
                                        gMemBufId);
  try {
    parser->parse(contentbuf);
  }
  catch (const xercesc::XMLException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.getMessage());
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Exception message is %C.\n"), message));
    xercesc::XMLString::release(&message);
    return -1;
  }
  catch (const xercesc::DOMException& toCatch) {
    char* message = xercesc::XMLString::transcode(toCatch.msg);
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Exception message is: %C.\n"), message));
    xercesc::XMLString::release(&message);
    return -1;
  }
  catch (...) {
    ACE_DEBUG((LM_ERROR, ACE_TEXT(
        "(%P|%t) AccessControlBuiltInImpl::load_permissions_file: Unexpected Permissions XML Parser Exception.\n")));
    return -1;
  }


  // Successfully parsed the permissions file

  xercesc::DOMDocument* xmlDoc = parser->getDocument();

  xercesc::DOMElement* elementRoot = xmlDoc->getDocumentElement();
  if ( !elementRoot ) throw(std::runtime_error( "empty XML document" ));


  //TODO:  WARNING - this implementation only supports 1 permissions/grant set
 // Different from governance from here forward
  // Find the validity rules
  xercesc::DOMNodeList * grantRules = xmlDoc->getElementsByTagName(xercesc::XMLString::transcode("grant"));

  //PermissionGrantRules grant_rules_list_holder_;

  for (XMLSize_t r = 0; r < grantRules->getLength(); r++) {
    PermissionGrantRule rule_holder_;

    // Pull out the grant name for this grant
    xercesc::DOMNamedNodeMap * rattrs = grantRules->item(r)->getAttributes();
    rule_holder_.grant_name = xercesc::XMLString::transcode(rattrs->item(0)->getTextContent());

    // Pull out subject name, validity, and default
    xercesc::DOMNodeList * grantNodes = grantRules->item(r)->getChildNodes();

    for ( XMLSize_t gn = 0; gn < grantNodes->getLength(); gn++) {
      char *g_tag = xercesc::XMLString::transcode(grantNodes->item(gn)->getNodeName());

      if (strcmp(g_tag, "subject_name") == 0) {
        rule_holder_.subject = xercesc::XMLString::transcode(grantNodes->item(gn)->getTextContent());
      } else if (strcmp(g_tag, "validity") == 0) {
        //Validity_t gn_validity;
        xercesc::DOMNodeList *validityNodes = grantNodes->item(gn)->getChildNodes();

        for (XMLSize_t vn = 0; vn < validityNodes->getLength(); vn++) {
          char *v_tag = xercesc::XMLString::transcode((validityNodes->item(vn)->getNodeName()));

          if (strcmp(v_tag, "not_before") == 0) {
            rule_holder_.validity.not_before = xercesc::XMLString::transcode(
                      (validityNodes->item(vn)->getTextContent()));
          } else if (strcmp(v_tag, "not_after") == 0) {
            rule_holder_.validity.not_after = xercesc::XMLString::transcode(
                      (validityNodes->item(vn)->getTextContent()));
          }
        }
      } else if (strcmp(g_tag, "default") == 0) {
        rule_holder_.default_permission = xercesc::XMLString::transcode(grantNodes->item(gn)->getTextContent());
      }
    }
    // Pull out allow/deny rules
    xercesc::DOMNodeList * adGrantNodes = grantRules->item(r)->getChildNodes();

    for (XMLSize_t gn = 0; gn < adGrantNodes->getLength(); gn++) {
      char *g_tag = xercesc::XMLString::transcode(adGrantNodes->item(gn)->getNodeName());

      if (strcmp(g_tag, "allow_rule") == 0 || strcmp(g_tag, "deny_rule") == 0) {
        PermissionTopicRule ptr_holder_;
        PermissionsPartition pp_holder_;

        ptr_holder_.ad_type = (strcmp(g_tag,"allow_rule") ==  0 ? ALLOW : DENY);
        pp_holder_.ad_type = (strcmp(g_tag, "allow_rule") == 0 ? ALLOW : DENY);

        xercesc::DOMNodeList * adNodeChildren = adGrantNodes->item(gn)->getChildNodes();

        for (XMLSize_t anc = 0; anc < adNodeChildren->getLength(); anc++) {
          char *anc_tag = xercesc::XMLString::transcode(adNodeChildren->item(anc)->getNodeName());

          if (strcmp(anc_tag, "domains") == 0) {   //domain list
            xercesc::DOMNodeList * domainIdNodes = adNodeChildren->item(anc)->getChildNodes();

            for (XMLSize_t did = 0; did < domainIdNodes->getLength(); did++) {
              if (strcmp("id" , xercesc::XMLString::transcode(domainIdNodes->item(did)->getNodeName())) == 0) {
                ptr_holder_.domain_list.insert(atoi(xercesc::XMLString::transcode(domainIdNodes->item(did)->getTextContent())));
                pp_holder_.domain_list.insert(atoi(xercesc::XMLString::transcode(domainIdNodes->item(did)->getTextContent())));
              }
              else if (strcmp("id_range", xercesc::XMLString::transcode(domainIdNodes->item(did)->getNodeName())) == 0) {
                int min_value = 0;
                int max_value = 0;
                xercesc::DOMNodeList * domRangeIdNodes = domainIdNodes->item(did)->getChildNodes();

                for (XMLSize_t drid = 0; drid < domRangeIdNodes->getLength(); drid++) {
                  if (strcmp("min", xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getNodeName())) == 0) {
                    min_value = atoi(xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getTextContent()));
                  }
                  else if (strcmp("max", xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getNodeName())) == 0) {
                    max_value = atoi(xercesc::XMLString::transcode(domRangeIdNodes->item(drid)->getTextContent()));

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

          } else if (ACE_OS::strcasecmp(anc_tag, "publish") == 0 || ACE_OS::strcasecmp(anc_tag, "subscribe") == 0) {   // pub sub nodes
            PermissionTopicPsRule anc_ps_rule_holder_;
            PermissionPartitionPs anc_ps_partition_holder_;

            anc_ps_rule_holder_.ps_type = (ACE_OS::strcasecmp(anc_tag,"publish") ==  0 ? PUBLISH : SUBSCRIBE);
            anc_ps_partition_holder_.ps_type = (ACE_OS::strcasecmp(anc_tag, "publish") == 0 ? PUBLISH : SUBSCRIBE);
            xercesc::DOMNodeList * topicListNodes = adNodeChildren->item(anc)->getChildNodes();

            for (XMLSize_t tln = 0; tln < topicListNodes->getLength(); tln++) {
              if (strcmp("topics" , xercesc::XMLString::transcode(topicListNodes->item(tln)->getNodeName())) == 0) {
                xercesc::DOMNodeList * topicNodes = topicListNodes->item(tln)->getChildNodes();

                for (XMLSize_t tn = 0; tn < topicNodes->getLength(); tn++) {
                  if (strcmp("topic", xercesc::XMLString::transcode(topicNodes->item(tn)->getNodeName())) == 0) {
                    anc_ps_rule_holder_.topic_list.push_back(xercesc::XMLString::transcode(topicNodes->item(tn)->getTextContent()));
                  }
                }

              }
              else if (strcmp("partitions", xercesc::XMLString::transcode(topicListNodes->item(tln)->getNodeName())) == 0) {
                xercesc::DOMNodeList * partitionNodes = topicListNodes->item(tln)->getChildNodes();

                for (XMLSize_t pn = 0; pn < partitionNodes->getLength(); pn++) {
                  if (strcmp("partition", xercesc::XMLString::transcode(partitionNodes->item(pn)->getNodeName())) == 0) {
                    anc_ps_partition_holder_.partition_list.push_back(xercesc::XMLString::transcode(partitionNodes->item(pn)->getTextContent()));
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

  delete parser;
  delete errHandler;
  return 0;
}


}
}
