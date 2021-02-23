//==============================================================
/**
 *  @file  XML_MemBuf_Intf.h
 *
 *
 *  @author Danilo C. Zanella (dczanella@gmail.com)
 */
//================================================================

#ifndef OPENDDS_DCPS_QOS_XML_HANDLER_XML_MEMBUF_INTF_H
#define OPENDDS_DCPS_QOS_XML_HANDLER_XML_MEMBUF_INTF_H
#include /**/ "ace/pre.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds_qos.hpp"
#include "XML_Intf.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "OpenDDS_XML_QOS_Handler_Export.h"
#include "ace/XML_Utils/XML_Typedefs.h"
#include "ace/XML_Utils/XML_Schema_Resolver.h"
#include "ace/XML_Utils/XML_Error_Handler.h"

namespace XML
{
  class XML_Typedef;
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  class OpenDDS_XML_QOS_Handler_Export QOS_XML_MemBuf_Handler :
    public QOS_XML_Handler
  {
  public:
    QOS_XML_MemBuf_Handler(void);

    ~QOS_XML_MemBuf_Handler(void);

    /**
     *
     * init
     *
     * The init method will open the file and will validate
     * it against the schema. It returns RETCODE_ERROR
     * when any error occurs during parsing
     *
     */
    DDS::ReturnCode_t
    init(const ACE_TCHAR * membuf);

    /**
     *
     * add_search_path will add a relative path to the XML
     * parsing library. The XML parsing library will use
     * this path to search for the schema
     *
     */
    void
    add_search_path(const ACE_TCHAR *environment,
      const ACE_TCHAR *relpath);

  private:
    // Schema resolver
    XML::XML_Schema_Resolver<XML::Environment_Resolver> * res_;

    // Error handler
    XML::XML_Error_Handler * eh_;

    // Parser
    XercesDOMParser * parser_;

    // Final DOMDocument that should be passed to
    // dds::reader::dds method
    DOMDocument * finalDoc_;

  };
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* DCPS_CONFIG_XML_MEMBUF_INTF_H */
