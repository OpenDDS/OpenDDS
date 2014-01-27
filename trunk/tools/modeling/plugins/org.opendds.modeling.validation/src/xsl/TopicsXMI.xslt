<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema" version="1.0">
  
  <xsl:include href="Default.xslt"/>
  
  <xsl:template match="xsd:extension/@base[.='domain:DomainEntity' and ancestor::*/@name='Topic']">
    <xsl:attribute name="base">topics:TopicDescription</xsl:attribute>
  </xsl:template>
  
</xsl:stylesheet>