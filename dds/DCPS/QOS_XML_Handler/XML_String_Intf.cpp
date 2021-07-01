#include "XML_String_Intf.h"
#include "xercesc/util/XercesDefs.hpp"
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/sax/SAXParseException.hpp"
#include "xercesc/util/TransService.hpp"
#include "ace/XML_Utils/XML_Typedefs.h"
#include "ace/XML_Utils/XML_Schema_Resolver.h"
#include "ace/XML_Utils/XML_Error_Handler.h"


#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  QOS_XML_String_Handler::QOS_XML_String_Handler(void) :
    QOS_XML_Handler(),
    res_(new XML::XML_Schema_Resolver<XML::Environment_Resolver>()),
    eh_(new XML::XML_Error_Handler()),
    parser_(0),
    finalDoc_(0)
  {
    try
    {
      // Call Initialize if not already called
      XMLPlatformUtils::Initialize();

      ////////////////
      // Create parser
      parser_ =  new XercesDOMParser();

      // Create a DomImplementation
      XMLCh* xmlver = XMLString::transcode("XML 1.0");
      DOMImplementation * domImpl = DOMImplementationRegistry::getDOMImplementation(xmlver);
      XMLString::release(&xmlver);

      // Create a temporary document with a generic element
      // it shall be replaced in future with final version
      XMLCh* namespaceURI = XMLString::transcode("");
       XMLCh* qualifiedName = XMLString::transcode("temp");
      finalDoc_ = domImpl->createDocument(namespaceURI, qualifiedName,0);
      XMLString::release(&namespaceURI);
      XMLString::release(&qualifiedName);

    } catch (const XMLException& toCatch) {

      char* message = XMLString::transcode(toCatch.getMessage());
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("QOS_XML_String_Handler::QOS_XML_String_Handler - ")
        ACE_TEXT ("Error during XML initialization! :\n<%C>\n"),
        message));
      XMLString::release(&message);
    }
  }

  QOS_XML_String_Handler::~QOS_XML_String_Handler(void)
  {
    if (finalDoc_ != 0)
      finalDoc_->release();
    delete parser_;
    delete res_;
    delete eh_;

    XMLPlatformUtils::Terminate();
  }

  DDS::ReturnCode_t
  QOS_XML_String_Handler::init(const ACE_TCHAR * membuf)
  {
    DDS::ReturnCode_t retcode = DDS::RETCODE_OK;
    try
    {
      // Create a InputSource to be used in parser with XML string
      XMLCh * conv = XMLString::transcode(ACE_TEXT_ALWAYS_CHAR (membuf));
      TranscodeToStr xmlTranscoded(conv,"utf-8");
      XMLString::release(&conv);
      MemBufInputSource xmlBuf(xmlTranscoded.str(), xmlTranscoded.length() ,"xml (in memory)");

      ///////////////////
      // Configure Parser
      //
      // Perform Namespace processing.
      parser_->setDoNamespaces (true);
      // Discard comment nodes in the document
      parser_->setCreateCommentNodes (false);
      // Disable datatype normalization. The XML 1.0 attribute value
      // normalization always occurs though.
      // parser_->setFeature (XMLUni::fgDOMDatatypeNormalization, true);

      // Do not create EntityReference nodes in the DOM tree. No
      // EntityReference nodes will be created, only the nodes
      // corresponding to their fully expanded sustitution text will be
      // created.
      parser_->setCreateEntityReferenceNodes (false);
      // Perform Validation
      parser_->setValidationScheme (xercesc::AbstractDOMParser::Val_Always);
      // Do not include ignorable whitespace in the DOM tree.
      parser_->setIncludeIgnorableWhitespace (false);
      // Enable the parser_'s schema support.
      parser_->setDoSchema (true);
      // Enable full schema constraint checking, including checking which
      // may be time-consuming or memory intensive. Currently, particle
      // unique attribution constraint checking and particle derivation
      // restriction checking are controlled by this option.
      parser_->setValidationSchemaFullChecking (true);
      // The parser_ will treat validation error as fatal and will exit.
      parser_->setValidationConstraintFatal (true);

      // Set resolver using auxiliary XML_Schema_Resolver
      parser_->setEntityResolver(res_);

      // Set XML Error handler
      parser_->setErrorHandler(eh_);

      // Parsing buffer
      try {
        parser_->parse(xmlBuf);
      }
      catch (const SAXParseException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_String_Handler::init - ")
          ACE_TEXT ("Exception message is: <%C>\n"),
          message));
        XMLString::release(&message);
        return DDS::RETCODE_ERROR;
      }
      catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_String_Handler::init - ")
          ACE_TEXT ("Exception message is: <%C>\n"),
          message));
        XMLString::release(&message);
        return DDS::RETCODE_ERROR;
      }
      catch (const DOMException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_String_Handler::init - ")
          ACE_TEXT ("Exception message is: <%C>\n"),
          message));
        XMLString::release(&message);
        return DDS::RETCODE_ERROR;
      }
      catch (...) {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("QOS_XML_String_Handler::init - ")
          ACE_TEXT ("Unexpected exception\n")
          ));
        return DDS::RETCODE_ERROR;
      }

      DOMDocument * initialDoc = parser_->getDocument();
      if (initialDoc == 0)
      {
        if (DCPS_debug_level > 1)
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("QOS_XML_String_Handler::init - ")
            ACE_TEXT ("Failed to parse string\n")
            ));
        }
        return DDS::RETCODE_ERROR;
      }

      // Find "dds" node using the opendds namespace
      XMLCh* ns = XMLString::transcode("http://www.omg.org/dds");
      XMLCh* ddstag = XMLString::transcode("dds");

      DOMNodeList * ddsNodeList = initialDoc->getElementsByTagNameNS(ns, ddstag);
      XMLString::release(&ns);
      XMLString::release(&ddstag);

      // Check if it could find tag and namespace
      if (ddsNodeList->getLength() == 0)
      {
        if (DCPS_debug_level > 1)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("QOS_XML_String_Handler::init - ")
              ACE_TEXT ("Could not find tag(dds) in namespace(http://www.omg.org/dds)\n")
              ));
          }
        return DDS::RETCODE_ERROR;
      }

      // Get the first node. It is expected to have only one
      DOMNode * ddsNode = ddsNodeList->item(0);
      if (DCPS_debug_level > 1)
      {
        char* message = XMLString::transcode(ddsNode->getNodeName());
        ACE_DEBUG ((LM_DEBUG,
          ACE_TEXT ("QOS_XML_String_Handler::init - ")
          ACE_TEXT ("Node name: <%C>\n"),
          message));
        XMLString::release(&message);
      }

      DOMNode * clone = finalDoc_->importNode(ddsNode, true);

      // Check if import was successful
      if (clone == 0)
      {
        if (DCPS_debug_level > 1)
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("QOS_XML_String_Handler::init - ")
            ACE_TEXT ("Failed to get pointer of imported node\n")
            ));
        }
        return DDS::RETCODE_ERROR;
      }
      // Replace root element by the cloned one. Thus the root element
      // shall be "dds" as required by dds::reader::dds function
      finalDoc_->replaceChild(clone,finalDoc_->getDocumentElement());

      ID_Map::TSS_ID_Map* TSS_ID_Map (ACE_Singleton<ID_Map::TSS_ID_Map, ACE_Null_Mutex>::instance());
      (*TSS_ID_Map)->reset ();

      profiles_ = dds::reader::dds(finalDoc_);

    }
    catch (const CORBA::Exception &ex)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("QOS_XML_String_Handler::init - ")
        ACE_TEXT ("Caught CORBA exception whilst parsing XML\n"),
        ex._info ().c_str ()));
      retcode = DDS::RETCODE_ERROR;
    }
    catch (...)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("QOS_XML_String_Handler::init - ")
        ACE_TEXT ("Unexpected exception whilst parsing XML.\n")
        ));
      retcode = DDS::RETCODE_ERROR;
    }
    return retcode;

  }

  void
  QOS_XML_String_Handler::add_search_path(const ACE_TCHAR *environment,
                                          const ACE_TCHAR *relpath)
  {
    res_->get_resolver().add_path(environment,relpath);
  }

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
