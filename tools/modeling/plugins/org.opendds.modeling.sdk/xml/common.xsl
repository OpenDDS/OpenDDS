<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xmi='http://www.omg.org/XMI'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'>
  <!--
    ** $Id$
    **
    ** Common stylesheet templates and variables
    **
    -->
<xsl:variable name="lower" select="'abcdefghijklmnopqrstuvwxyz'"/>
<xsl:variable name="upper" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>

<xsl:template name="normalize-identifier">
  <xsl:param name="identifier" select="@name"/>
  <xsl:value-of select="translate($identifier, ' -', '__')"/>
</xsl:template>

<xsl:template name="type-enum">
  <xsl:param name="type" select="."/>
  <xsl:choose>
    <xsl:when test="string-length($type/../@model) &gt; 0">
      <xsl:value-of select="concat($type/../@model, '_', $type/@name)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$type/@name"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="capitalize">
  <xsl:param name="value"/>
  <xsl:variable name="first" select="substring($value, 1, 1)"/>
  <xsl:variable name="FIRST" select="translate($first, $lower, $upper)"/>
  <xsl:variable name="rest" select="substring($value, 2)"/>
  <xsl:value-of select="concat($FIRST, $rest)"/>
</xsl:template>
</xsl:stylesheet>

