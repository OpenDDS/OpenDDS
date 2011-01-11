<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:lut='http://www.opendds.com/modeling/schemas/Lut/1.0'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.org/modeling/schemas/Generator/1.0'>
  <!--
    ** $Id$
    **
    ** Generate C++ implementation code.
    **
    ** @TODO - check string management.
    ** @TODO - determine how to set the transport addresses.
    **
    -->
<xsl:include href="common.xsl"/>

<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<!-- Extract the name of the model once. -->
<xsl:variable name="modelname" select="document(/generator:CodeGen/source/@name)//opendds:OpenDDSModel/@name"/>
<xsl:variable name="MODELNAME" select="translate($modelname, $lower, $upper)"/>

<xsl:template match="/">
  <xsl:value-of select="concat('#ifndef ', $MODELNAME, '_TRAITS_H', $newline)"/>
  <xsl:value-of select="concat('#define ', $MODELNAME, '_TRAITS_H', $newline)"/>
  <xsl:value-of select="concat('#include &quot;', $modelname, '_T.h&quot;', $newline)"/>

  <xsl:apply-templates/>
<xsl:text>
#endif
</xsl:text>
</xsl:template>

<xsl:template match="//instance">
  <xsl:variable name="Instname">
    <xsl:call-template name="capitalize">
      <xsl:with-param name="value" select="@name"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="structname" select="concat($Instname, 
                                                 $modelname, 
                                                 'Traits')"/>
  <xsl:variable name="export" select="concat($modelname, '_Export ')"/>
  <xsl:variable name="tdname" select="concat($Instname, $modelname, 'Type')"/>

  <!-- output struct declaration -->
  <xsl:value-of select="concat($newline,
    'struct ', $export, $structname, 
    ' : OpenDDS::Model::DefaultInstanceTraits {', $newline,
    '  enum { transport_key_base = ', transportOffset/@value, '};', $newline,
    '  void transport_config(OpenDDS::DCPS::TransportIdType id);', $newline,
    '};', $newline
  )"/>

  <!-- output typedef-->
  <xsl:value-of select="concat($newline,
    'typedef OpenDDS::Model::Service&lt; OpenDDS::Model::', $modelname,
    '::Elements, ', $structname, '&gt; ', $tdname, ';', $newline
  )"/>

  <!-- output enum -->

</xsl:template>
</xsl:stylesheet>
