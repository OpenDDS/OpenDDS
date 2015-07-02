<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.org/modeling/schemas/Generator/1.0'>
  <!--
    **
    ** Generate paths MPB file.
    **
    -->
<xsl:include href="common.xsl"/>

<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="model" select="document(/generator:CodeGen/source/@name)/opendds:OpenDDSModel"/>
<xsl:variable name="modelname" select="$model//@name"/>

<!-- process the entire model document to produce the MPC base for external paths. -->
<xsl:template match="/">
  <xsl:value-of select="concat('project {', $newline)"/>
  <xsl:for-each select="/generator:CodeGen/searchPaths/searchLocation">
    <xsl:variable name="path">
      <xsl:if test="variable/@value">
        <xsl:value-of select="concat('$(', variable/@value, ')')"/>
      </xsl:if>
      <xsl:if test="path/@value">
        <xsl:if test="variable/@value">
          <xsl:value-of select="'/'"/>
        </xsl:if>
        <xsl:value-of select="path/@value"/>
      </xsl:if>
    </xsl:variable>

    <xsl:value-of select="concat('  libpaths      +=   ', $path, $newline)"/>
    <xsl:value-of select="concat('  includes      +=   ', $path, $newline)"/>
    <xsl:value-of select="concat('  idlflags      += -I', $path, $newline)"/>
    <xsl:value-of select="concat('  dcps_ts_flags += -I', $path, $newline)"/>
  </xsl:for-each>
  <xsl:value-of select="concat('}', $newline)"/>
</xsl:template>
<!-- End of main processing template. -->

</xsl:stylesheet>

