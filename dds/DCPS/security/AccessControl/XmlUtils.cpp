/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "XmlUtils.h"

#include <ace/OS_NS_strings.h>

#include <xercesc/util/XMLDateTime.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace XmlUtils {

using DDS::Security::DomainId_t;

ParserPtr get_parser(const std::string& xml, const std::string& filename)
{
  try {
    xercesc::XMLPlatformUtils::Initialize();

  } catch (const xercesc::XMLException& ex) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
      "XMLPlatformUtils::Initialize XMLException: %C\n",
      to_string(ex).c_str()));
    return ParserPtr();
  }

  ParserPtr parser(new xercesc::XercesDOMParser());
  parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);
  parser->setCreateCommentNodes(false);
  parser->setIncludeIgnorableWhitespace(false);
  xercesc::MemBufInputSource contentbuf(
    reinterpret_cast<const XMLByte*>(xml.c_str()), xml.size(), filename.c_str());

  try {
    parser->parse(contentbuf);

  } catch (const xercesc::XMLException& ex) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
      "XMLException while parsing \"%C\": %C\n",
      filename.c_str(), to_string(ex).c_str()));
    return ParserPtr();

  } catch (const xercesc::DOMException& ex) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
      "DOMException while parsing \"%C\": %C\n",
      filename.c_str(), to_string(ex).c_str()));
    return ParserPtr();

  } catch (...) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
      "Unexpected exception while parsing \"%C\"",
      filename.c_str()));
    return ParserPtr();
  }

  if (!parser->getDocument()->getDocumentElement()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
      "XML document \"%C\" is empty\n",
      filename.c_str()));
    return ParserPtr();
  }

  return parser;
}

std::string to_string(const XMLCh* in)
{
  char* c = xercesc::XMLString::transcode(in);
  const std::string s(c);
  xercesc::XMLString::release(&c);
  return s;
}

bool to_bool(const XMLCh* in, bool& value)
{
  /*
   * The security spec specifies the XML schema spec's boolean type, but the
   * XML schema spec doesn't specify that true and false can be capitalized
   * like the security spec uses.
   */
  const std::string s = to_string(in);
  if (!ACE_OS::strcasecmp(s.c_str(), "true") || s == "1") {
    value = true;
  } else if (!ACE_OS::strcasecmp(s.c_str(), "false") || s == "0") {
    value = false;
  } else {
    return false;
  }
  return true;
}

bool to_time(const XMLCh* in, time_t& value)
{
  try {
    xercesc::XMLDateTime xdt(in);
    xdt.parseDateTime();
    value = xdt.getEpoch();
  } catch (const xercesc::XMLException& ex) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: to_time: failed to parse date/time: %C\n",
      to_string(ex).c_str()));
    return false;
  }
  return true;
}

namespace {
  bool to_domain_id(const xercesc::DOMNode* node, DomainId_t& value)
  {
    // NOTE: DomainId_t is a signed type, but a domain id actually can't be a negative value.
    unsigned int i = 0;
    const bool success = xercesc::XMLString::textToBin(node->getTextContent(), i);
    if (success) {
      value = static_cast<DomainId_t>(i);
    }
    return success;
  }
}

bool to_domain_id_set(const xercesc::DOMNode* node, DomainIdSet& domain_id_set)
{
  const xercesc::DOMNodeList* const domainIdNodes = node->getChildNodes();
  for (XMLSize_t did = 0, did_len = domainIdNodes->getLength(); did < did_len; ++did) {
    const xercesc::DOMNode* const domainIdNode = domainIdNodes->item(did);
    if (!is_element(domainIdNode)) {
      continue;
    }
    const std::string domainIdNodeName = to_string(domainIdNode->getNodeName());
    if (domainIdNodeName == "id") {
      DomainId_t domain_id;
      if (!to_domain_id(domainIdNode, domain_id)) {
        ACE_ERROR((LM_ERROR,
          "(%P|%t) ERROR: to_domain_id_set: Invalid domain ID \"%C\" in id\n",
          to_string(domainIdNode).c_str()));
        return false;
      }
      domain_id_set.insert(domain_id);

    } else if (domainIdNodeName == "id_range") {
      DomainId_t min_value = 0;
      DomainId_t max_value = 0;
      const xercesc::DOMNodeList* const domRangeIdNodes = domainIdNode->getChildNodes();
      for (XMLSize_t drid = 0, drid_len = domRangeIdNodes->getLength(); drid < drid_len; ++drid) {
        const xercesc::DOMNode* const domRangeIdNode = domRangeIdNodes->item(drid);
        if (!is_element(domRangeIdNode)) {
          continue;
        }
        const std::string domRangeIdNodeName = to_string(domainIdNode->getNodeName());
        if ("min" == domRangeIdNodeName) {
          if (!to_domain_id(domRangeIdNode, min_value)) {
            ACE_ERROR((LM_ERROR,
              "(%P|%t) ERROR: to_domain_id_set: Invalid domain ID \"%C\" in min_value\n",
              to_string(domainIdNode).c_str()));
            return false;
          }

        } else if ("max" == domRangeIdNodeName) {
          if (!to_domain_id(domRangeIdNode, max_value)) {
            ACE_ERROR((LM_ERROR,
              "(%P|%t) ERROR: to_domain_id_set: Invalid domain ID \"%C\" in max_value\n",
              to_string(domainIdNode).c_str()));
            return false;
          }

          if (min_value > max_value || min_value == 0) {
            ACE_ERROR((LM_ERROR,
              "(%P|%t) ERROR: to_domain_id_set: Permission XML Domain Range invalid.\n"));
            return false;
          }

          for (DomainId_t i = min_value; i <= max_value; ++i) {
            domain_id_set.insert(i);
          }
        }
      }

    } else {
      ACE_ERROR((LM_ERROR,
        "(%P|%t) ERROR: to_domain_id_set: Invalid tag \"%C\" in domain ID set: \"%C\"\n",
        domainIdNodeName.c_str(), to_string(domainIdNode).c_str()));
      return false;
    }
  }

  if (domain_id_set.empty()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: to_domain_id_set: empty domain ID set\n"));
    return false;
  }

  return true;
}

} // namespace XmlUtils
} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
