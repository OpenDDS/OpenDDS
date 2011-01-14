<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
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

<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

<!--
  A::B::C::D::name
-->
<xsl:template name="scopename">
  <xsl:param name="target" select="."/>

  <xsl:choose>
    <xsl:when test="not($target)">
    </xsl:when>
    <xsl:when test="name($target) = 'opendds:OpenDDSModel'">
    </xsl:when>
    <xsl:when test="name($target) = 'external-refs'">
    </xsl:when>
    <xsl:when test="name($target) = 'dataLib'">
      <xsl:call-template name="scopename">
        <xsl:with-param name="target" select="$target/.."/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="name($target) = 'packages'">
      <xsl:variable name="prefix">
        <xsl:call-template name="scopename">
          <xsl:with-param name="target" select="$target/.."/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat($prefix, $target/@name, '::')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="scopename">
        <xsl:with-param name="target" select="$target/.."/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="normalize-identifier">
  <xsl:param name="identifier" select="@name"/>
  <xsl:value-of select="translate($identifier, ' -:', '___')"/>
</xsl:template>

<xsl:template name="type-enum">
  <xsl:param name="type" select="."/>
  <xsl:call-template name="normalize-identifier">
    <xsl:with-param name="identifier" select="$type/@name"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="capitalize">
  <xsl:param name="value"/>
  <xsl:variable name="first" select="substring($value, 1, 1)"/>
  <xsl:variable name="FIRST" select="translate($first, $lower, $upper)"/>
  <xsl:variable name="rest" select="substring($value, 2)"/>
  <xsl:value-of select="concat($FIRST, $rest)"/>
</xsl:template>

 <xsl:template name="string-replace-all">
    <xsl:param name="text" />
    <xsl:param name="replace" />
    <xsl:param name="by" />
    <xsl:choose>
      <xsl:when test="contains($text, $replace)">
        <xsl:value-of select="substring-before($text,$replace)" />
        <xsl:value-of select="$by" />
        <xsl:call-template name="string-replace-all">
          <xsl:with-param name="text"
          select="substring-after($text,$replace)" />
          <xsl:with-param name="replace" select="$replace" />
          <xsl:with-param name="by" select="$by" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

<xsl:template name="elements-qname">
  <xsl:param name="target" select="."/>
  <xsl:param name="previous" select="''"/>

  <xsl:choose>
    <xsl:when test="name($target) = 'opendds:OpenDDSModel'">
      <xsl:value-of select="concat('OpenDDS::Model::', $previous, '::Elements')"/>
    </xsl:when>
    <xsl:when test="name($target) = 'libs' and 
                    $target[@xsi:type = 'opendds:DcpsLib']">
      <xsl:call-template name="elements-qname">
        <xsl:with-param name="target" select="$target/.."/>
        <xsl:with-param name="previous" select="$target/@name"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="name($target) = 'packages'">
      <xsl:call-template name="elements-qname">
        <xsl:with-param name="target" select="$target/.."/>
        <xsl:with-param name="previous" select="concat($target/@name, '::', $previous)"/>
      </xsl:call-template>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template name="data-qname">
  <xsl:call-template name="elements-qname"/>
  <xsl:text>::Data</xsl:text>
</xsl:template>

<xsl:template name="model-ref-names">
  <xsl:param name="model-refs" select="//@model"/>
  <xsl:param name="complete-refs" select="''"/>

  <xsl:if test="$model-refs">
    <xsl:variable name="model-ref" select="$model-refs[1]"/>
    <xsl:if test="not(contains($complete-refs, $model-ref))">
      <xsl:if test="string-length($complete-refs) &gt; 0">
        <xsl:text>, </xsl:text>
      </xsl:if>
      <xsl:value-of select="$model-ref"/>
    </xsl:if>
    <xsl:call-template name="model-ref-names">
      <xsl:with-param name="model-refs" select="$model-refs[position() &gt; 1]"/>
      <xsl:with-param name="complete-refs" select="concat(' ', $model-ref, ' ')"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>
</xsl:stylesheet>

