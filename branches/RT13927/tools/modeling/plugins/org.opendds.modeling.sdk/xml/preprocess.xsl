<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xmi='http://www.omg.org/XMI'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'>
  <!--
    ** $Id$
    **
    ** Generate C++ header code.
    **
    -->

<xsl:variable name="topics"       select="//topics"/>

<xsl:template match="opendds:OpenDDSModel">
  <xsl:copy>
    <xsl:apply-templates select="@*"/>
    <xsl:apply-templates/>
    <xsl:call-template name="process-external-ref-topics">
      <xsl:with-param name="reffing-topics" select="$topics[datatype[@href]]"/>
    </xsl:call-template>
  </xsl:copy>
</xsl:template>

<xsl:template match="*[*[@href]]">
  <xsl:copy>
    <xsl:apply-templates select="@*"/>
    <xsl:for-each select="*[@href]">
      <xsl:call-template name="replace-href"/>
    </xsl:for-each>
  </xsl:copy>
</xsl:template>

<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>

<xsl:template name="replace-href">
  <xsl:choose>
    <xsl:when test="name() = 'datatype'">
      <xsl:attribute name="datatype">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>Unknown href container element <xsl:value-of select="name()"/></xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="process-external-ref-topics">
  <xsl:param name="reffing-topics"/>
  <xsl:param name="complete-types" select="' '"/>

  <xsl:if test="$reffing-topics">
    <xsl:variable name="topic" select="$reffing-topics[1]"/>
    <xsl:variable name="external-id" select="substring-after($topic/datatype/@href, '#')"/>
    <xsl:if test="not(contains($complete-types, concat(' ',$external-id,' ')))">
      <xsl:variable name="external-model" select="substring-before($topic/datatype/@href, '#')"/>
      <xsl:for-each select="document($external-model)//types[@xmi:id = $external-id]">
        <xsl:element name="dataLib">
          <xsl:attribute name="model">
            <xsl:value-of select="../../@name"/>
          </xsl:attribute>
          <xsl:apply-templates select="../@name"/>
          <xsl:copy>
            <xsl:apply-templates select="@name | @xmi:id"/>
          </xsl:copy>
        </xsl:element>
      </xsl:for-each>
    </xsl:if>

    <xsl:call-template name="process-external-ref-topics">
      <xsl:with-param name="reffing-topics" select="$reffing-topics[position() > 1]"/>
      <xsl:with-param name="complete-types" select="concat($complete-types, ' ', $external-id, ' ')"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
