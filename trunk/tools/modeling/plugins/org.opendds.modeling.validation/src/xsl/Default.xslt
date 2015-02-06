<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema" version="1.0">
  <xsl:output method="xml" indent="yes"/>
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
  <!-- this is needed if doing validation outside of the eclipse plugin, 
    but we also need to provide a copy of the XMI.xsd document -->
  <xsl:template match="xsd:import[@namespace='http://www.omg.org/XMI']">
    <xsl:element name="xsd:import">
      <xsl:attribute name="namespace">http://www.omg.org/XMI</xsl:attribute>
      <xsl:attribute name="schemaLocation">XMI.xsd</xsl:attribute>
    </xsl:element>
  </xsl:template>
  
  <!-- kill top level elements that don't represent valid instance documents -->
  <xsl:template match="/xsd:schema/xsd:element[@name != 'OpenDDSModel' 
                                               and @name != 'CodeGen'
                                               and not(starts-with(@type,'xmi:'))]"/>
  
  <!-- fix all the broken schemmaLocation values, the documents should be FooXMI.xsd instead of foo.xsd -->
    <xsl:template match="@schemaLocation">
    <xsl:choose>
       <xsl:when test=".='opendds.xsd'">
        <xsl:attribute name="schemaLocation">OpenDDSXMI.xsd</xsl:attribute>
       </xsl:when>
      <xsl:when test=".='qos.xsd'">
        <xsl:attribute name="schemaLocation">QoSXMI.xsd</xsl:attribute>
      </xsl:when>
      <xsl:when test=".='application.xsd'">
        <xsl:attribute name="schemaLocation">ApplicationXMI.xsd</xsl:attribute>
      </xsl:when>
      <xsl:when test=".='dcps.xsd'">
        <xsl:attribute name="schemaLocation">DCPSXMI.xsd</xsl:attribute>
      </xsl:when>
       <xsl:when test=".='types.xsd'">
        <xsl:attribute name="schemaLocation">TypesXMI.xsd</xsl:attribute>
      </xsl:when>
       <xsl:when test=".='topics.xsd'">
        <xsl:attribute name="schemaLocation">TopicsXMI.xsd</xsl:attribute>
      </xsl:when>
       <xsl:when test=".='domain.xsd'">
        <xsl:attribute name="schemaLocation">DomainXMI.xsd</xsl:attribute>
      </xsl:when>
       <xsl:when test=".='enumerations.xsd'">
        <xsl:attribute name="schemaLocation">EnumerationsXMI.xsd</xsl:attribute>
       </xsl:when>
       <xsl:when test=".='core.xsd'">
        <xsl:attribute name="schemaLocation">CoreXMI.xsd</xsl:attribute>
       </xsl:when>
      <xsl:otherwise>
        <xsl:attribute name="schemaLocation"><xsl:value-of select="."/></xsl:attribute>
      </xsl:otherwise>
   </xsl:choose>
  </xsl:template>
 
</xsl:stylesheet>