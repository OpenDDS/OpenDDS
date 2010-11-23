<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:lut='http://www.opendds.com/modeling/schemas/Lut/1.0'
     xmlns:opendds='http://www.opendds.com/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.com/modeling/schemas/Generator/1.0'>
  <!--
    ** $Id$
    **
    ** Generate C++ implementation code.
    **
    ** @TODO - check string management.
    ** @TODO - determine how to set the transport addresses.
    **
    -->
<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "'JJS'"/>
<xsl:variable name = "MODELNAME" select = "translate($modelname, 
                                           'abcdefghijklmnopqrstuvwxyz',
                                           'ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">
  <xsl:message>Hard-coding model name</xsl:message>
  <xsl:value-of select="concat('#ifndef ', $MODELNAME, '_TRAITS_H', $newline)"/>
  <xsl:value-of select="concat('#define ', $MODELNAME, '_TRAITS_H', $newline)"/>
  <xsl:variable name="classname" select="concat($modelname, 'Traits')"/>

  <xsl:text>
#include "model/DefaultInstanceTraits.h"

struct </xsl:text><xsl:value-of select="$classname"/><xsl:text> : OpenDDS::Model::DefaultInstanceTraits {
  void transport_config(OpenDDS::DCPS::TransportIdType id);
};

#endif
</xsl:text>
</xsl:template>
</xsl:stylesheet>
