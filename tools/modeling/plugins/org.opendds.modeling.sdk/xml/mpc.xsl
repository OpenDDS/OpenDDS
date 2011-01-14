<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.org/modeling/schemas/Generator/1.0'>
  <!--
    ** $Id$
    **
    ** Generate MPC implementation code.
    **
    -->
<xsl:include href="common.xsl"/>

<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<!-- Node sets -->
<xsl:variable name="dcpslib" select="//libs[@xsi:type = 'opendds:DcpsLib']"/>

<!-- Extract the name of the model once. -->
<xsl:variable name="modelname" select="/opendds:OpenDDSModel/@name"/>

<xsl:variable name="MODELNAME" select="translate($modelname, $lower, $upper)"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">
  <xsl:variable name="model-refs">
    <xsl:call-template name="model-ref-names"/>
  </xsl:variable>
  <xsl:text>project(</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>): dcps</xsl:text>
  
  <xsl:if test="string-length($model-refs) &gt; 0">
    <xsl:value-of select="concat(', ', $model-refs)"/>
  </xsl:if>
  <xsl:text> {
  libout = .
  sharedname = </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>
  includes += $(DDS_ROOT)/tools/modeling/codegen

  idlflags      += -Wb,export_macro=</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_Export -Wb,export_include=</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_export.h
  dynamicflags   = </xsl:text>
  <xsl:value-of select="$MODELNAME"/>
  <xsl:text>_BUILD_DLL
  dcps_ts_flags += -Wb,export_macro=</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>_Export
  prebuild      += perl $(DDS_ROOT)/bin/expfile.pl </xsl:text>
  <xsl:value-of select="$modelname"/>
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

