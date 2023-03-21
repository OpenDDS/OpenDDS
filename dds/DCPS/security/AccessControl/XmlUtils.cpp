/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "XmlUtils.h"

#include <dds/DCPS/debug.h>

#include <ace/OS_NS_strings.h>
#include <ace/OS_NS_time.h>

#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/util/XMLDateTime.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XercesVersion.hpp>

#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace XmlUtils {

using DDS::Security::DomainId_t;
using OpenDDS::DCPS::security_debug;

std::string to_string(const xercesc::SAXParseException& ex)
{
  return
    to_string(ex.getSystemId()) +
    ":" + DCPS::to_dds_string(ex.getLineNumber()) +
    ":" + DCPS::to_dds_string(ex.getColumnNumber()) +
    ": " + to_string(ex.getMessage());
}

namespace {
  class ErrorHandler : public xercesc::ErrorHandler {
  public:
    void warning(const xercesc::SAXParseException& ex)
    {
      if (security_debug.access_warn) {
        ACE_ERROR((LM_ERROR, "(%P|%t) WARNING: {access_warn} "
          "XmlUtils::ErrorHandler: %C\n",
          to_string(ex).c_str()));
      }
    }

    void error(const xercesc::SAXParseException& ex)
    {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} "
          "XmlUtils::ErrorHandler: %C\n",
          to_string(ex).c_str()));
      }
    }

    void fatalError(const xercesc::SAXParseException& ex)
    {
      error(ex);
    }

    void resetErrors()
    {
    }
  };
}

bool get_parser(ParserPtr& parser, const std::string& filename, const std::string& xml)
{
  try {
    xercesc::XMLPlatformUtils::Initialize();

  } catch (const xercesc::XMLException& ex) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} get_parser: "
        "XMLPlatformUtils::Initialize XMLException: %C\n",
        to_string(ex).c_str()));
    }
    parser.reset();
    return false;
  }

  parser.reset(new xercesc::XercesDOMParser());

  parser->setDoNamespaces(true);
  parser->setIncludeIgnorableWhitespace(false);
  parser->setCreateCommentNodes(false);

  ErrorHandler error_handler;
  parser->setErrorHandler(&error_handler);

  xercesc::MemBufInputSource contentbuf(
    reinterpret_cast<const XMLByte*>(xml.c_str()), xml.size(), filename.c_str());

  try {
    parser->parse(contentbuf);

  } catch (const xercesc::XMLException& ex) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} get_parser: "
        "XMLException while parsing \"%C\": %C\n",
        filename.c_str(), to_string(ex).c_str()));
    }
    parser.reset();
    return false;

  } catch (const xercesc::DOMException& ex) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} get_parser: "
        "DOMException while parsing \"%C\": %C\n",
        filename.c_str(), to_string(ex).c_str()));
    }
    parser.reset();
    return false;

  } catch (...) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} get_parser: "
        "Unexpected exception while parsing \"%C\"",
        filename.c_str()));
    }
    parser.reset();
    return false;
  }

  if (!parser->getDocument()->getDocumentElement()) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} get_parser: "
        "XML document \"%C\" is empty\n",
        filename.c_str()));
    }
    parser.reset();
    return false;
  }

  return true;
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

#if _XERCES_VERSION >= 30200
#  define XMLDATETIME_HAS_GETEPOCH 1
#else
#  define XMLDATETIME_HAS_GETEPOCH 0
namespace {
  bool parse_time_string(
    const std::string& whole, size_t& pos, std::string& value, size_t width = std::string::npos)
  {
    const bool to_end = width == std::string::npos;
    const size_t size = whole.size();
    if (pos >= size || (!to_end && pos + width > size)) {
      return false;
    }
    value = whole.substr(pos, width);
    pos = to_end ? size : pos + width;
    return true;
  }

  bool parse_time_field(
    const std::string& whole, size_t& pos, int& value, size_t width = 2)
  {
    std::string string;
    if (parse_time_string(whole, pos, string, width)) {
      if (OpenDDS::DCPS::convertToInteger(string, value) && string[0] != '-') {
        return true;
      }
    }
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_time_field: "
        "failed to get field at pos %B in \"%C\"\n",
        pos, whole.c_str()));
    }
    return false;
  }

  bool parse_time_char_or_end(
    const std::string& whole, size_t& pos, const std::string& choices, char& c, bool& end)
  {
    std::string string;
    if (parse_time_string(whole, pos, string, 1)) {
      end = false;
      c = string[0];
      if (choices.find(c) != std::string::npos) {
        return true;
      }
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_time_char_or_end: "
          "failed to find one of \"%C\" at pos %B in \"%C\"\n",
          choices.c_str(), pos, whole.c_str()));
      }
      return false;
    }
    end = true;
    return true;
  }

  bool parse_time_char(
    const std::string& whole, size_t& pos, const std::string& choices)
  {
    char c;
    bool end;
    if (!parse_time_char_or_end(whole, pos, choices, c, end)) {
      return false;
    }
    if (end) {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_time_char: "
          "failed to find one of \"%C\" at end of \"%C\"\n",
          choices.c_str(), whole.c_str()));
      }
      return false;
    }
    return true;
  }
}
#endif // XMLDATETIME_HAS_GETEPOCH

bool parse_time(const XMLCh* in, time_t& value)
{
  xercesc::XMLDateTime xdt(in);
  try {
    xdt.parseDateTime();
  } catch (const xercesc::XMLException& ex) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_time: "
        "failed to parse date/time \"%C\": %C\n",
        to_string(in).c_str(), to_string(ex).c_str()));
    }
    return false;
  }

#if XMLDATETIME_HAS_GETEPOCH
  value = xdt.getEpoch();
#else
  /*
   * Doesn't seem like older Xerces' actually have a way to get the information
   * from XMLDateTime, so we have to do it ourselves.
   *
   * For this we'll follow https://www.w3.org/TR/xmlschema-2/#dateTime, which
   * is basically the same as ISO8601.
   * The exceptions are:
   * - We won't accept a negative datetime (-0001 is the year 2BCE), since the
   *   standard library might not be able to handle such a far back and we're
   *   certainly not expecting it.
   * - The Schema spec says that times without timezone info should be
   *   interpreted as "local", but seems it be intentionally vague as what that
   *   means. The DDS Security 1.1 spec explicitly says in 9.4.1.3.2.2 that it
   *   would be UTC. Xerces getEpoch values match up with this in the unit test
   *   for this code, so it's also interpreting them as UTC instead of
   *   something like the local system timezone.
   * - Since we use time_t we will accept but ignore fractional seconds.
   *   NOTE: The security spec is missing fractional seconds in a format
   *   description in a comment in example XML. This comment has been copied
   *   into many permissions files sitting in OpenDDS.
   */

  const std::string str = to_string(in);
  if (str[0] == '-') {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_time: "
        "date/time can't be negative: \"%C\"\n",
        str.c_str()));
    }
    return false;
  }

  /*
   * After this the expected lexical format given by the Schema spec is:
   *   yyyy '-' mm '-' dd 'T' hh ':' mm ':' ss ('.' s+)? ((('+' | '-') hh ':' mm) | 'Z')?
   * or alternatively as the Security spec says, except with fractional seconds:
   *   CCYY-MM-DDThh:mm:ss[.fffff...][Z|(+|-)hh:mm]
   * See unit tests for this code for examples
   */
  std::tm dttm;
  size_t pos = 0;

  // Year
  if (!parse_time_field(str, pos, dttm.tm_year, 4)) {
    return false;
  }
  dttm.tm_year -= 1900;
  if (!parse_time_char(str, pos, "-")) {
    return false;
  }
  // Month
  if (!parse_time_field(str, pos, dttm.tm_mon)) {
    return false;
  }
  --dttm.tm_mon;
  if (!parse_time_char(str, pos, "-")) {
    return false;
  }
  // Day
  if (!parse_time_field(str, pos, dttm.tm_mday)) {
    return false;
  }
  if (!parse_time_char(str, pos, "T")) {
    return false;
  }
  // Hour
  if (!parse_time_field(str, pos, dttm.tm_hour)) {
    return false;
  }
  if (!parse_time_char(str, pos, ":")) {
    return false;
  }
  // Minutes
  if (!parse_time_field(str, pos, dttm.tm_min)) {
    return false;
  }
  if (!parse_time_char(str, pos, ":")) {
    return false;
  }
  // Seconds
  if (!parse_time_field(str, pos, dttm.tm_sec)) {
    return false;
  }

  // Ignore Fractional Seconds
  if (str[pos] == '.') {
    ++pos;
    for (; str[pos] >= '0' && str[pos] <= '9'; ++pos) {
    }
  }

  // Optional Timezone Info
  time_t timezone_offset = 0;
  char tz_char = '\0';
  bool end;
  if (!parse_time_char_or_end(str, pos, "+-Z", tz_char, end)) {
    return false;
  }
  if (!end && tz_char != 'Z') {
    int tz_hour;
    if (!parse_time_field(str, pos, tz_hour)) {
      return false;
    }

    if (!parse_time_char(str, pos, ":")) {
      return false;
    }

    int tz_min;
    if (!parse_time_field(str, pos, tz_min)) {
      return false;
    }

    /*
     * We will have to do the reverse of the sign to convert the local time to
     * UTC. For example 00:00:00-06:00 (12AM CST) is 06:00:00+00:00 (6AM UTC).
     */
    const int timezone_offset_sign = tz_char == '+' ? -1 : 1;

    /*
     * We could modify the tm struct here to account for the timezone, but then
     * we'd also have to adjust the hour if the minutes carried over and the
     * date fields if the hour carried over to another date. Instead we're
     * going to calculate the number of seconds in the offset and add that to
     * the time_t value later.
     */
    timezone_offset = timezone_offset_sign * (tz_hour * 60 * 60 + tz_min * 60);
  }

  std::string leftover;
  if (parse_time_string(str, pos, leftover)) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_time: "
        "leftover characters in \"%C\": \"%C\"\n",
        str.c_str(), leftover.c_str()));
    }
    return false;
  }

  dttm.tm_isdst = 0; // Don't try to do anything with DST
  value = std::mktime(&dttm);
  if (value == time_t(-1)) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_time: "
        "failed to convert to time_t \"%C\"\n",
        to_string(in).c_str()));
    }
    return false;
  }

  // mktime assumes the tm struct is in the local time, so we need to correct
  // for that.
  const time_t local_timezone = static_cast<time_t>(ace_timezone());
  if (local_timezone == 0 && errno == ENOTSUP) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_time: "
        "ace_timezone not supported\n"));
    }
    return false;
  }
  value -= local_timezone;

  // Adjust for the timezone specified in the string if there was one.
  value += timezone_offset;
#endif // XMLDATETIME_HAS_GETEPOCH

  return value;
}

namespace {
  bool parse_domain_id(const xercesc::DOMNode* node, DomainId_t& value)
  {
    // NOTE: DomainId_t is a signed type, but a domain id actually can't be a negative value.
    unsigned int i = 0;
    const bool success = xercesc::XMLString::textToBin(node->getTextContent(), i) &&
      i <= static_cast<unsigned>(domain_id_max);
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
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_domain_id_set: "
            "Invalid domain ID \"%C\" in id\n",
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
              ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_domain_id_set: "
                "Invalid domain ID \"%C\" in min_value\n",
                to_string(domRangeIdNode).c_str()));
            }
            return false;
          }
          has_min = true;

        } else if ("max" == domRangeIdNodeName && !has_max) {
          if (!parse_domain_id(domRangeIdNode, max_value)) {
            if (security_debug.access_error) {
              ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_domain_id_set: "
                "Invalid domain ID \"%C\" in max_value\n",
                to_string(domRangeIdNode).c_str()));
            }
            return false;
          }
          if (min_value > max_value) {
            if (security_debug.access_error) {
              ACE_ERROR((LM_ERROR,"(%P|%t) ERROR: {access_error} parse_domain_id_set: "
                "Permission XML Domain Range invalid.\n"));
            }
            return false;
          }
          has_max = true;

        } else {
          if (security_debug.access_error) {
            ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_domain_id_set: "
              "Invalid tag \"%C\" in id_range\n",
              domRangeIdNodeName.c_str()));
          }
          return false;
        }
      }

      domain_id_set.add(min_value, max_value);

    } else {
      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_domain_id_set: "
          "Invalid tag \"%C\" in domain ID set\n",
          domainIdNodeName.c_str()));
      }
      return false;
    }
  }

  if (domain_id_set.empty()) {
    if (security_debug.access_error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} parse_domain_id_set: "
        "empty domain ID set\n"));
    }
    return false;
  }

  return true;
}

} // namespace XmlUtils
} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
