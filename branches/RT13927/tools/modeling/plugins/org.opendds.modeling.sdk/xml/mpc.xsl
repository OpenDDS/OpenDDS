<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.org/modeling/schemas/Generator/1.0'>
  <!--
    ** $Id$
    **
    ** Generate MPC implementation code.
    **
    -->
<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<!-- blech -->
<xsl:variable name="lowercase" select="'abcdefghijklmnopqrstuvwxyz'"/>
<xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>

<!-- Node sets -->
<xsl:variable name="application" select="//opendds:application"/>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "/opendds:OpenDDSModel/@name"/>

<xsl:variable name = "MODELNAME" select = "translate( $modelname, $lowercase, $uppercase)"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">
  <xsl:text>project(</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>): dcps {
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

}
</xsl:text>
</xsl:template>
<!-- End of main processing template. -->

</xsl:stylesheet>

