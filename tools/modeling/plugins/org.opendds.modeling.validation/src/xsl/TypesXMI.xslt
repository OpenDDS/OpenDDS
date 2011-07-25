<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema" version="1.0">

  <xsl:include href="Default.xslt"/>

  <!-- change the String,WString, Sequence types so they don't require the length attribute
from
  <xsd:complexType name="String">
    <xsd:complexContent>
      <xsd:extension base="types:Collection"/>
    </xsd:complexContent>
  </xsd:complexType>

to
  <xsd:complexType abstract="true" name="Collection">
    <xsd:complexContent>
      <xsd:extension base="types:Type">
        <xsd:attribute name="length" type="xsd:long" use="required"/>
      </xsd:extension>
    </xsd:complexContent>
  </xsd:complexType>
  
note, the Sequence types has extra extension defs
  <xsd:complexType name="Sequence">
    <xsd:complexContent>
      <xsd:extension base="types:Collection">
        <xsd:choice maxOccurs="unbounded" minOccurs="0">
          <xsd:element name="subtype" type="types:Type"/>
        </xsd:choice>
        <xsd:attribute name="subtype" type="xsd:string"/>
      </xsd:extension>
    </xsd:complexContent>
  </xsd:complexType>
    -->

  <xsl:template match="xsd:schema/xsd:complexType[@name='String' or @name='WString' or @name='Sequence']">
    <xsl:element name="xsd:complexType">
      <xsl:attribute name="name">
        <xsl:value-of select="@name"/>
      </xsl:attribute>
      <xsl:element name="xsd:complexContent">
        <xsl:element name="xsd:extension">
          <xsl:attribute name="base">types:Type</xsl:attribute>
          <!-- if it's a Sequence, it needs extra definitions -->
          <xsl:if test="@name='Sequence'">
            <xsl:element name="xsd:choice">
              <xsl:attribute name="maxOccurs">unbounded</xsl:attribute>
              <xsl:attribute name="minOccurs">0</xsl:attribute>
              <xsl:element name="xsd:element">
                <xsl:attribute name="name">subtype</xsl:attribute>
                <xsl:attribute name="type">types:Type</xsl:attribute>
              </xsl:element>
            </xsl:element>
            <xsl:element name="xsd:attribute">
              <xsl:attribute name="name">subtype</xsl:attribute>
              <xsl:attribute name="type">xsd:string</xsl:attribute>
            </xsl:element>
          </xsl:if>
          <!-- they all need the optional length attr -->
          <xsl:element name="xsd:attribute">
            <xsl:attribute name="name">length</xsl:attribute>
            <xsl:attribute name="type">xsd:long</xsl:attribute>
            <xsl:attribute name="use">optional</xsl:attribute>
            <xsl:attribute name="default">0</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
    </xsl:element>
  </xsl:template>
</xsl:stylesheet>
