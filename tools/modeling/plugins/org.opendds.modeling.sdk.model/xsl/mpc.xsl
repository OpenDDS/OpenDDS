<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.org/modeling/schemas/Generator/1.0'>
  <!--
    **
    ** Generate MPC implementation code.
    **
    -->
<xsl:include href="common.xsl"/>

<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="model" select="document(/generator:CodeGen/source/@name)/opendds:OpenDDSModel"/>

<!-- Extract the name of the model once. -->
<xsl:variable name="modelname" select="$model/@name"/>
<xsl:variable name="normalized-modelname">
  <xsl:call-template name="normalize-identifier">
    <xsl:with-param name="identifier" select="$model/@name"/>
  </xsl:call-template>
</xsl:variable>

<xsl:variable name="MODELNAME" select="translate($modelname, $lower, $upper)"/>

<!-- process the entire genfile document to produce the MPC file. -->
<xsl:template match="/">
  <xsl:variable name="dcpslib" select="$model//libs[@xsi:type = 'opendds:DcpsLib']"/>
  <xsl:variable name="model-refs">
    <xsl:call-template name="data-model-ref-names">
      <xsl:with-param name="model-refs" select="$model//datatype/@href | $model//topic/@href | $model//libs[@xsi:type='types:DataLib']//@href"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:text>project(</xsl:text>
  <xsl:value-of select="concat($modelname, ') : dcps, all_dcps_transports, ', $modelname, '_paths')"/>
  
  <xsl:if test="string-length($model-refs) &gt; 0">
    <xsl:value-of select="concat(', ', $model-refs)"/>
  </xsl:if>
  <xsl:text> {
  libout = .
  sharedname = </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>
  includes += $(DDS_ROOT)/tools/modeling/codegen

  <!--
    Suppress DCPS_DATA_TYPE warnings for now. The proper thing to do would be
    to modify idl.xsl to use topic type annotations.
  -->
  dcps_ts_flags += --no-dcps-data-type-warnings

  idlflags      += -Wb,export_macro=</xsl:text>
  <xsl:value-of select="$normalized-modelname"/>
  <xsl:text>_Export -Wb,export_include=</xsl:text>
  <xsl:value-of select="$normalized-modelname"/>
  <xsl:text>_export.h
  dynamicflags   = </xsl:text>
  <xsl:value-of select="$MODELNAME"/>
  <xsl:text>_BUILD_DLL
  dcps_ts_flags += -Wb,export_macro=</xsl:text>
  <xsl:value-of select="$normalized-modelname"/>
  <xsl:text>_Export
  prebuild      += perl $(DDS_ROOT)/bin/expfile.pl </xsl:text>
  <xsl:value-of select="concat($modelname, $newline, $newline)"/>
  <xsl:text>
  TypeSupport_Files {
    </xsl:text>
    <xsl:value-of select="$modelname"/>
    <xsl:text>.idl
  }

  IDL_Files {
    </xsl:text>
    <xsl:value-of select="$modelname"/>
    <xsl:text>.idl
  }

  Header_Files {
</xsl:text>
    <xsl:value-of select="concat('    ', $modelname, '_T.h', $newline)"/>
    <xsl:if test="$dcpslib">
      <xsl:value-of select="concat('    ', $modelname, 'Traits.h', $newline)"/>
    </xsl:if>
    <xsl:text>  }

  Source_Files {
</xsl:text>
    <xsl:if test="$dcpslib">
      <xsl:value-of select="concat('    ', $modelname, 'Traits.cpp', $newline)"/>
    </xsl:if>
    <xsl:text>  }

  Template_Files {
    </xsl:text>
    <xsl:value-of select="$modelname"/>
    <xsl:text>_T.cpp
  }
}
</xsl:text>
</xsl:template>
<!-- End of main processing template. -->

</xsl:stylesheet>

