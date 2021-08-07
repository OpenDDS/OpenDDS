/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "XmlUtils.h"

#include <dds/DCPS/debug.h>

#include <ace/OS_NS_strings.h>
#include <ace/OS_NS_time.h>

#include <xercesc/util/XMLDateTime.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XercesVersion.hpp>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace XmlUtils {

using DDS::Security::DomainId_t;
using OpenDDS::DCPS::security_debug;

ParserPtr get_parser(const std::string& filename, const std::string& xml)
{
  try {
    xercesc::XMLPlatformUtils::Initialize();

  } catch (const xercesc::XMLException& ex) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
        "XMLPlatformUtils::Initialize XMLException: %C\n",
        to_string(ex).c_str()));
    }
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
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
        "XMLException while parsing \"%C\": %C\n",
        filename.c_str(), to_string(ex).c_str()));
    }
    return ParserPtr();

  } catch (const xercesc::DOMException& ex) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
        "DOMException while parsing \"%C\": %C\n",
        filename.c_str(), to_string(ex).c_str()));
    }
    return ParserPtr();

  } catch (...) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
        "Unexpected exception while parsing \"%C\"",
        filename.c_str()));
    }
    return ParserPtr();
  }

  if (!parser->getDocument()->getDocumentElement()) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: get_parser: "
        "XML document \"%C\" is empty\n",
        filename.c_str()));
    }
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

bool parse_bool(const XMLCh* in, bool& value)
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

bool parse_time(const XMLCh* in, time_t& value)
{
  xercesc::XMLDateTime xdt(in);
  try {
    xdt.parseDateTime();
  } catch (const xercesc::XMLException& ex) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: parse_time: failed to parse date/time \"%C\": %C\n",
        to_string(in).c_str(), to_string(ex).c_str()));
    }
    return false;
  }

#if _XERCES_VERSION >= 30200
  value = xdt.getEpoch();
#else
  std::tm xdt_tm;
  xdt_tm.tm_year = xdt.getYear() - 1900;
  xdt_tm.tm_mon = xdt.getMonth() - 1;
  xdt_tm.tm_mday = xdt.getDay();
  xdt_tm.tm_hour = xdt.getHour();
  xdt_tm.tm_min = xdt.getMinute();
  xdt_tm.tm_sec = xdt.getSecond();
  xdt_tm.tm_isdst = 0;
  value = std::mktime(&xdt_tm);
  if (value == time_t(-1)) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: parse_time: failed to convert to time_t \"%C\"\n",
        to_string(in).c_str()));
    }
    return false;
  }

  const time_t the_timezone = static_cast<time_t>(ace_timezone());
  if (the_timezone == 0 && errno == ENOTSUP) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: parse_time: ace_timezone not supported\n"));
    }
    return false;
  }
  value -= the_timezone;
#endif

  return value;
}

namespace {
  bool parse_domain_id(const xercesc::DOMNode* node, DomainId_t& value)
  {
    // NOTE: DomainId_t is a signed type, but a domain id actually can't be a negative value.
    unsigned int i = 0;
    const bool success = xercesc::XMLString::textToBin(node->getTextContent(), i) &&
      i <= domain_id_max;
    if (success) {
      value = static_cast<DomainId_t>(i);
    }
    return success;
  }
}

bool parse_domain_id_set(const xercesc::DOMNode* node, Security::DomainIdSet& domain_id_set)
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
      if (!parse_domain_id(domainIdNode, domain_id)) {
        if (security_debug.access_error) {
          ACE_ERROR((LM_ERROR,
            "(%P|%t) ERROR: parse_domain_id_set: Invalid domain ID \"%C\" in id\n",
            to_string(domainIdNode).c_str()));
        }
        return false;
      }
      domain_id_set.add(domain_id);

    } else if (domainIdNodeName == "id_range") {
      const xercesc::DOMNodeList* const domRangeIdNodes = domainIdNode->getChildNodes();
      DomainId_t min_value = domain_id_min;
      bool has_min = false;
      DomainId_t max_value = domain_id_max;
      bool has_max = false;
      const XMLSize_t drid_len = domRangeIdNodes->getLength();
      for (XMLSize_t drid = 0; drid < drid_len; ++drid) {
        const xercesc::DOMNode* const domRangeIdNode = domRangeIdNodes->item(drid);
        if (!is_element(domRangeIdNode)) {
          continue;
        }
        const std::string domRangeIdNodeName = to_string(domRangeIdNode->getNodeName());
        if ("min" == domRangeIdNodeName && !has_min) {
          if (!parse_domain_id(domRangeIdNode, min_value)) {
            if (security_debug.access_error) {
              ACE_ERROR((LM_ERROR,
                "(%P|%t) ERROR: parse_domain_id_set: Invalid domain ID \"%C\" in min_value\n",
                to_string(domRangeIdNode).c_str()));
            }
            return false;
          }
          has_min = true;

        } else if ("max" == domRangeIdNodeName && !has_max) {
          if (!parse_domain_id(domRangeIdNode, max_value)) {
            if (security_debug.access_error) {
              ACE_ERROR((LM_ERROR,
                "(%P|%t) ERROR: parse_domain_id_set: Invalid domain ID \"%C\" in max_value\n",
                to_string(domRangeIdNode).c_str()));
            }
            return false;
          }
          if (min_value > max_value) {
            if (security_debug.access_error) {
              ACE_ERROR((LM_ERROR,
                "(%P|%t) ERROR: parse_domain_id_set: Permission XML Domain Range invalid.\n"));
            }
            return false;
          }
          has_max = true;

        } else {
          if (security_debug.access_error) {
            ACE_ERROR((LM_ERROR,
              "(%P|%t) ERROR: parse_domain_id_set: Invalid tag \"%C\" in id_range\n",
              domRangeIdNodeName.c_str()));
          }
          return false;
        }
      }

      domain_id_set.add(min_value, max_value);

    } else {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR,
          "(%P|%t) ERROR: parse_domain_id_set: Invalid tag \"%C\" in domain ID set\n",
          domainIdNodeName.c_str()));
      }
      return false;
    }
  }

  if (domain_id_set.empty()) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: parse_domain_id_set: empty domain ID set\n"));
    }
    return false;
  }

  return true;
}

} // namespace XmlUtils
} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
