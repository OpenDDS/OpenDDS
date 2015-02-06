<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xsd="http://www.w3.org/2001/XMLSchema" version="1.0">
  
  <xsl:include href="Default.xslt"/>
  
  <!-- two elements are defined that are extensions of opendds:qosPolicy, this doesn't work for
    instance documents, do use qos:QosPolicy as extension base instead -->
  <xsl:template match="xsd:element/@type[.='opendds:qosPolicy' and ancestor::*/@base='opendds:OpenDDSLib']">
    <xsl:attribute name="type">qos:QosPolicy</xsl:attribute>
  </xsl:template>
  
</xsl:stylesheet>