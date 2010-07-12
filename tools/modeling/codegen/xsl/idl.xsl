<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:lut='http://www.opendds.com/modeling/schemas/Lut/1.0'
     xmlns:opendds='http://www.opendds.com/modeling/schemas/OpenDDS/1.0'
     xmlns:generator='http://www.opendds.com/modeling/schemas/Generator/1.0'>
  <!--
    ** $Id$
    **
    ** Generate IDL code.
    **
    ** @TODO Determine how unions are to be specified and represented.
    **
    -->
<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- Documents -->
<xsl:variable name="lut" select="document('')/*/lut:tables"/>

<!-- Node sets -->
<xsl:variable name="type"     select="//opendds:type"/>
<xsl:variable name="enum"     select="//opendds:type[ @type = 'opendds:idlEnum']"/>
<xsl:variable name="union"    select="//opendds:type[ @type = 'opendds:idlUnion']"/>
<xsl:variable name="struct"   select="//opendds:type[ @type = 'opendds:idlStruct']"/>
<xsl:variable name="array"    select="//opendds:type[ @type = 'opendds:idlArray']"/>
<xsl:variable name="sequence" select="//opendds:type[ @type = 'opendds:idlSequence']"/>
<xsl:variable name="typedef"  select="//opendds:type[ @type = 'opendds:idlTypedef']"/>

<!-- Index (lookup table is at the bottom of this document) -->
<xsl:key
     name  = "lut-type"
     match = "type"
     use   = "@type"/>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "/generator:model/@name"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">

  <xsl:text>
module </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text> {
</xsl:text>

  <!-- Generate IDL for enumerations. -->
  <xsl:apply-templates select="$enum">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>

  <!-- Generate IDL to forward declare unions. -->
  <xsl:value-of select="$newline"/>
  <xsl:apply-templates select="$union" mode="declare">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>

  <!-- Generate IDL to forward declare structures. -->
  <xsl:value-of select="$newline"/>
  <xsl:apply-templates select="$struct" mode="declare">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>

  <!-- Generate IDL for typedefs. -->
  <xsl:value-of select="$newline"/>
  <xsl:apply-templates select="$typedef">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>

  <!-- Generate IDL for arrays. -->
  <xsl:apply-templates select="$array">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>

  <!-- Generate IDL for sequences. -->
  <xsl:apply-templates select="$sequence">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>

  <!-- Generate IDL for unions. -->
  <xsl:apply-templates select="$union" mode="define">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>

  <!-- Generate IDL for structures. -->
  <xsl:apply-templates select="$struct" mode="define">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>

  <xsl:text>
};

</xsl:text>
</xsl:template>
<!-- End of main processing template. -->

<!-- Process each enumeration definition. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlEnum']">
  <xsl:value-of select="$newline"/>
  <xsl:text>  enum </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>  {</xsl:text>
  <xsl:value-of select="$newline"/>

  <xsl:apply-templates select="./opendds:member" mode="enum"/>

  <xsl:text>  };</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Forward declare each union definition. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlUnion']" mode="declare">
  <xsl:text>  union </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>;</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Forward declare each data structure definition. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlStruct']" mode="declare">
  <xsl:text>  struct </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>;</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Process each typedef definition. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlTypedef']">
  <!-- 'typedef (target/@type) (@name);\n' -->
  <xsl:text>  typedef </xsl:text>
  <xsl:call-template name="typespec">
    <xsl:with-param name="spectype" select="opendds:member/@type"/>
  </xsl:call-template>
  <xsl:text> </xsl:text>
  <xsl:value-of select="opendds:member/@name"/>
  <xsl:text>;</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Process each array definition. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlArray']">
  <!-- 'typedef (member/@type) (member/@name)[ (member/@size)];\n' -->
  <xsl:text>  typedef </xsl:text>
  <xsl:call-template name="typespec">
    <xsl:with-param name="spectype" select="opendds:member/@type"/>
  </xsl:call-template>
  <xsl:text> </xsl:text>
  <xsl:value-of select="opendds:member/@name"/>
  <xsl:text>[ </xsl:text>
  <xsl:value-of select="opendds:member/@size"/>
  <xsl:text>];</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Process each sequence definition. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlSequence']">
  <!-- 'typedef sequence<(member/@type)> (@member/name);\n' -->
  <xsl:text>  typedef sequence&lt;</xsl:text>
  <xsl:call-template name="typespec">
    <xsl:with-param name="spectype" select="opendds:member/@type"/>
  </xsl:call-template>
  <xsl:text>&gt; </xsl:text>
  <xsl:value-of select="opendds:member/@name"/>
  <xsl:text>;</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Process each union definition. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlUnion']" mode="define">
  <xsl:value-of select="$newline"/>
  <xsl:text>  union </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>  switch (</xsl:text>
  <xsl:call-template name="typespec">
    <xsl:with-param name="spectype" select="@switch"/>
  </xsl:call-template>
  <xsl:text>)  {</xsl:text>
  <xsl:value-of select="$newline"/>

  <xsl:apply-templates select="./opendds:case"/>

  <xsl:if test="./opendds:default">
    <xsl:text>    default: </xsl:text>
    <xsl:call-template name="typespec">
      <xsl:with-param name="spectype" select="./opendds:default/@type"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <xsl:value-of select="./opendds:default/@name"/>
    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline"/>
  </xsl:if>

  <xsl:text>  };</xsl:text>
  <xsl:value-of select="$newline"/>


</xsl:template>

<!-- Process each data structure definition. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlStruct']" mode="define">
  <!-- '#pragma DCPS_DATA_TYPE "(modelname)::(type/@name)"\n' -->
  <xsl:value-of select="$newline"/>
  <xsl:text>#pragma DCPS_DATA_TYPE "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>"</xsl:text>
  <xsl:value-of select="$newline"/>

  <xsl:for-each select="./opendds:key">
    <xsl:call-template name="pragma-key">
      <xsl:with-param name="key" select="."/>
    </xsl:call-template>
  </xsl:for-each>

  <xsl:text>  struct </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>  {</xsl:text>
  <xsl:value-of select="$newline"/>

  <xsl:apply-templates select="./opendds:member" mode="struct"/>

  <xsl:text>  };</xsl:text>
  <xsl:value-of select="$newline"/>

</xsl:template>

<!-- Process union cases. -->
<xsl:template match="opendds:case">
  <!-- Build the output string for the type specification. -->
  <xsl:variable name="typespec">
    <xsl:call-template name="typespec">
      <xsl:with-param name="spectype" select="@type"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- '  (@type) (@name);\n' -->
  <xsl:text>    case </xsl:text>
  <xsl:value-of select="@label"/>
  <xsl:text>: </xsl:text>
  <xsl:value-of select="$typespec"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>;</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Process structure members. -->
<xsl:template match="opendds:member" mode="struct">
  <!-- Build the output string for the type specification. -->
  <xsl:variable name="typespec">
    <xsl:call-template name="typespec">
      <xsl:with-param name="spectype" select="@type"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- '  (@type) (@name);\n' -->
  <xsl:text>    </xsl:text>
  <xsl:value-of select="$typespec"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>;</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Process enumeration members. -->
<xsl:template match="opendds:member" mode="enum">
  <xsl:text>    </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:if test="position() != last()">
    <xsl:text>,</xsl:text>
  </xsl:if>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Create a DCPS_DATA_KEY pragma line. -->
<xsl:template name="pragma-key">
  <xsl:param name="key"/>

  <!-- '#pragma DCPS_DATA_KEY "(modelname)::(type/@name) (member)"\n' -->
  <xsl:text>#pragma DCPS_DATA_KEY  "</xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text>::</xsl:text>
  <xsl:value-of select="../@name"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="@member"/>
  <xsl:text>"</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Produce a type specification string for a type. -->
<xsl:template name="typespec">
  <xsl:param name="spectype"/>

  <!-- Extract the CORBA type if it is one. -->
  <xsl:variable name="corbatype">
    <xsl:call-template name="corbatype">
      <xsl:with-param name="name" select="$spectype"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- Build the output string for the type specification. -->
  <xsl:choose>
    <!-- CORBA types require no additional processing. -->
    <xsl:when test="string-length($corbatype) > 0">
      <xsl:value-of select="$corbatype"/>
    </xsl:when>

    <!-- Process user defined types here. -->
    <xsl:when test="$type[@name=$spectype]">
      <xsl:choose>
        <!-- Typedef and Sequence types use the translated (defined) type name. -->
        <xsl:when test="$type[@name=$spectype]/@type = 'opendds:idlTypedef'
                     or $type[@name=$spectype]/@type = 'opendds:idlArray'
                     or $type[@name=$spectype]/@type = 'opendds:idlSequence'">
          <xsl:value-of select="$type[@name=$spectype]/opendds:member/@name"/>
        </xsl:when>

        <!-- Structs, Unions, and Enums use the struct name. -->
        <xsl:when test="$type[@name=$spectype]/@type = 'opendds:idlStruct'
                     or $type[@name=$spectype]/@type = 'opendds:idlUnion'
                     or $type[@name=$spectype]/@type = 'opendds:idlEnum'
                       ">
          <xsl:value-of select="$spectype"/>
        </xsl:when>

        <!-- Its an unknown user defined type. -->
        <xsl:otherwise>
          <xsl:text>UNKNOWN USER TYPE </xsl:text>
          <xsl:value-of select="$spectype"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <!-- Its neither a CORBA type nor user defined type. -->
    <xsl:otherwise>
      <xsl:text>UNKNOWN TYPE </xsl:text>
      <xsl:value-of select="$spectype"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Convert a model type to a CORBA IDL type. -->
<xsl:template name="corbatype">
  <xsl:param name="name"/>

  <xsl:variable name="typename">
    <xsl:call-template name="basename">
      <xsl:with-param name="name" select="$name"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:for-each select="$lut"> <!-- Change context for the lookup -->
    <xsl:value-of select="normalize-space(key('lut-type', $typename)/@corbatype)"/>
  </xsl:for-each>
</xsl:template>

<!-- Strip any namespace qualifier from a variable. -->
<xsl:template name="basename">
  <xsl:param name="name"/>
  <xsl:choose>
    <!-- Strip the namespace qualifier from the name. -->
    <xsl:when test="contains($name,':')">
      <xsl:value-of select="substring-after($name,':')"/>
    </xsl:when>

    <!-- Nothing to do. -->
    <xsl:otherwise>
      <xsl:value-of select="$name"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Lookup Table Magic. -->
<xsl:variable name="lookupTables" select="document('')/*//lut:types"/>
<lut:tables>
  <type type="Boolean"   cpptype="bool"               corbatype="boolean"/>
  <type type="Char"      cpptype="char"               corbatype="char"/>
  <type type="Octet"     cpptype="unsigned char"      corbatype="octet"/>
  <type type="Double"    cpptype="double"             corbatype="double"/>
  <type type="Float"     cpptype="float"              corbatype="float"/>
  <type type="Short"     cpptype="short"              corbatype="short"/>
  <type type="Integer"   cpptype="int"                corbatype="long"/>
  <type type="Long"      cpptype="long"               corbatype="long"/>
  <type type="LongLong"  cpptype="long long"          corbatype="long long"/>
  <type type="UShort"    cpptype="unsigned short"     corbatype="unsigned short"/>
  <type type="UInteger"  cpptype="unsigned int"       corbatype="unsigned long"/>
  <type type="ULong"     cpptype="unsigned long"      corbatype="unsigned long"/>
  <type type="ULongLong" cpptype="unsigned long long" corbatype="unsigned long long"/>
  <type type="String"    cpptype="char*"              corbatype="string"/>
  <type type="Array"                                  corbatype="array"/>
  <type type="Sequence"                               corbatype="sequence"/>
  <type type="Union"                                  corbatype="union"/>
  <type type="Enum"                                   corbatype="enum"/>
  <type type="Struct"                                 corbatype="struct"/>
</lut:tables>
<!-- ................... -->

</xsl:stylesheet>

