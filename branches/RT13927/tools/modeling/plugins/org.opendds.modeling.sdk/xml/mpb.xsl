<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.org/modeling/schemas/Generator/1.0'>
  <!--
    ** $Id$
    **
    ** Generate MPB file.
    **
    -->
<xsl:include href="common.xsl"/>

<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<!-- Extract the name of the model once. -->
<xsl:variable name="modelname" select="/opendds:OpenDDSModel/@name"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">
  <xsl:variable name="model-refs">
    <xsl:call-template name="model-ref-names"/>
  </xsl:variable>
  
  <xsl:text>project</xsl:text>
  <xsl:if test="string-length($model-refs) &gt; 0">
    <xsl:text> : </xsl:text>
  </xsl:if>
  <xsl:value-of select="concat($model-refs, ' {', $newline)"/>
  <xsl:value-of select="concat('  libs += ', $modelname, $newline)"/>
  <xsl:value-of select="concat('  after += ', $modelname, $newline)"/>
  <xsl:value-of select="concat('}', $newline)"/>
</xsl:template>
<!-- End of main processing template. -->

</xsl:stylesheet>

