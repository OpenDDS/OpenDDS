<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:lut='http://www.opendds.com/modeling/schemas/Lut/1.0'
     xmlns:opendds='http://www.opendds.com/modeling/schemas/OpenDDS/1.0'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
     xmlns:xmi='http://www.omg.org/XMI'>
  <!--
    ** $Id$
    **
    ** Generate IDL code.
    **
    -->
<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- Documents -->
<xsl:variable name="lut" select="document('lut.xml')/*/lut:types"/>

<!-- Node sets -->
<xsl:variable name="type"     select="//types"/>

<!-- Index (lookup table is in lut variable) -->
<xsl:key
     name  = "lut-type"
     match = "type"
     use   = "@type"/>

<!-- Extract the name of the model once. -->
<xsl:variable name = "modelname" select = "//datalib/@name"/>

<!-- process the entire model document to produce the C++ code. -->
<xsl:template match="/">

  <!-- required to build on windows -->
  <xsl:call-template name="processIntrinsicSequences"/>

  <xsl:text>
// Some header information should be generated here.
module </xsl:text>
  <xsl:value-of select="$modelname"/>
  <xsl:text> {
</xsl:text>

  <!-- Generate IDL for forward declarations. -->
  <xsl:apply-templates select="$type" mode="declare">
    <xsl:sort select="@name"/>
  </xsl:apply-templates>
  <xsl:value-of select="$newline"/>

  <!-- Terminal user defined types are all unreferenced user defined types -->
  <xsl:variable name="terminals" select="$type[not(@xmi:id  = $type//@type or @xmi:id=$type//@subtype or @xmi:id=$type//@switch)]"/>

  <!-- Process all types with no dependencies on them. -->
  <xsl:call-template name="generate-idl">
    <xsl:with-param name="nodes" select="$terminals"/>
  </xsl:call-template>

  <xsl:text>

};

</xsl:text>
</xsl:template>
<!-- End of main processing template. -->

<!-- Depth first traversal of type nodes processing predecessors first. -->
<xsl:template name="generate-idl">
  <xsl:param name="nodes"/>     <!-- <opendds:type> element nodes -->
  <xsl:param name="excluded"/>  <!-- Space separated string of types already processed. -->

  <!--
    ** We can't just apply-templates here as we need to keep track of what
    ** we have already output.
    -->
  <xsl:for-each select="$nodes">
    <!-- Collect the previously output types for exclusion.  -->
    <xsl:variable name="curpos" select="position()"/>
    <xsl:variable name="priors" select="$nodes[position() &lt; $curpos]"/>
    <xsl:variable name="exclude-list">
      <xsl:for-each select="$priors">
        <xsl:call-template name="get-dependencies"/>
      </xsl:for-each>
      <xsl:text> </xsl:text>
      <xsl:value-of select="$excluded"/>
    </xsl:variable>

    <!-- Process new predecessor types. -->
    <xsl:variable name="direct-predecessors" select="$type[@xmi:id = current()/*/@type]"/>
    <xsl:call-template name="generate-idl">
      <xsl:with-param name="nodes"
           select="$direct-predecessors[not(contains($exclude-list,concat(' ',@name,' ')))]"/>
      <xsl:with-param name="excluded" select="$exclude-list"/>
    </xsl:call-template>

    <!--
      ** Actually generate the IDL for this node after its predecessors
      ** have been processed.
      -->
    <xsl:apply-templates select="."/>
  </xsl:for-each>
</xsl:template>

<!-- Depth first search for type names of predecessors. -->
<xsl:template name="get-dependencies">
  <!-- successors is the set of nodes already represented in the get-dependencies output.
       used to prevent duplicates. -->
  <xsl:param name="successors" select="/.."/>

  <xsl:variable name="direct-predecessors" select="$type[@xmi:id = current()/*/@type]"/>

  <!-- using a Kaysian intersection predicate causes issues in eclipse here -->
  <xsl:for-each select="$direct-predecessors">
    <xsl:if test="not($successors[@name = current()/@name])">
      <xsl:call-template name="get-dependencies">
        <!-- Kaysian intersection are fine in eclipse due to no context switch -->
        <xsl:with-param name="successors" select=". | $successors"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:for-each>
  <xsl:text> </xsl:text>
  <xsl:value-of select="@name"/>
</xsl:template>

<!-- Forward declare union definitions. -->
<xsl:template match="types[@xsi:type = 'types:Union']" mode="declare">
  <xsl:text>  union </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>;</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Forward declare data structure definitions. -->
<xsl:template match="types[@xsi:type = 'types:Struct']" mode="declare">
  <xsl:text>  struct </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>;</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Process enumeration definitions. -->
<xsl:template match="types[ @xsi:type = 'types:Enum']">
  <xsl:value-of select="$newline"/>
  <xsl:text>  enum </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>  {</xsl:text>
  <xsl:value-of select="$newline"/>

  <xsl:apply-templates select="literals" mode="enum"/>

  <xsl:text>  };</xsl:text>
  <xsl:value-of select="$newline"/>
</xsl:template>

<!-- Process typedef definitions. -->
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

<!-- Process array definitions. -->
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

<!-- Process sequence definitions. -->
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

<!-- Process union definitions. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlUnion']">
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

<!-- Process data structure definitions. -->
<xsl:template match="opendds:type[ @type = 'opendds:idlStruct']">
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

<!-- Process individual union cases. -->
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

<!-- Process individual structure members. -->
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
<xsl:template match="literals" mode="enum">
  <xsl:text>    </xsl:text>
  <xsl:value-of select="."/>
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

<!--
  <xsl:choose>
    <xsl:when test="$typename='Boolean'">boolean</xsl:when>
    <xsl:when test="$typename='Char'">char</xsl:when>
    <xsl:when test="$typename='WChar'">wchar</xsl:when>
    <xsl:when test="$typename='Octet'">octet</xsl:when>
    <xsl:when test="$typename='Double'">double</xsl:when>
    <xsl:when test="$typename='Float'">float</xsl:when>
    <xsl:when test="$typename='Short'">short</xsl:when>
    <xsl:when test="$typename='Integer'">long</xsl:when>
    <xsl:when test="$typename='Long'">long</xsl:when>
    <xsl:when test="$typename='LongLong'">long long</xsl:when>
    <xsl:when test="$typename='UShort'">unsigned short</xsl:when>
    <xsl:when test="$typename='UInteger'">unsigned long</xsl:when>
    <xsl:when test="$typename='ULong'">unsigned long</xsl:when>
    <xsl:when test="$typename='ULongLong'">unsigned long long</xsl:when>
    <xsl:when test="$typename='String'">string</xsl:when>
    <xsl:when test="$typename='WString'">wstring</xsl:when>
    <xsl:when test="$typename='Array'">array</xsl:when>
    <xsl:when test="$typename='Sequence'">sequence</xsl:when>
    <xsl:when test="$typename='Union'">union</xsl:when>
    <xsl:when test="$typename='Enum'">enum</xsl:when>
    <xsl:when test="$typename='Struct'">struct</xsl:when>
  </xsl:choose>
-->
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
  <type type="WChar"                                  corbatype="wchar"/>
  <type type="WString"                                corbatype="wstring"/>
</lut:tables>
<!-- ................... -->

<xsl:template name="processIntrinsicSequences">

  <!-- pull tests to output each include only once -->
  <xsl:variable name="sequences" select="$type[@type = 'opendds:idlSequence']/opendds:member"/>
  <xsl:if test="$sequences[@type = 'opendds:Boolean']">
    <xsl:text>#include &lt;tao/BooleanSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:Char']">
    <xsl:text>#include &lt;tao/CharSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:WChar']">
    <xsl:text>#include &lt;tao/WCharSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:Octet']">
    <xsl:text>#include &lt;tao/Octet.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:Double']">
    <xsl:text>#include &lt;tao/DoubleSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:Float']">
    <xsl:text>#include &lt;tao/FloatSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:Short']">
    <xsl:text>#include &lt;tao/ShortSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:Long' or @type = 'opendds:Integer']">
    <xsl:text>#include &lt;tao/LongSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:LongLong']">
    <xsl:text>#include &lt;tao/LongLongSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:UShort']">
    <xsl:text>#include &lt;tao/UShortSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:ULong' or @type = 'opendds:UInteger']">
    <xsl:text>#include &lt;tao/ULongSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:ULongLong']">
    <xsl:text>#include &lt;tao/ULongLongSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:String']">
    <xsl:text>#include &lt;tao/StringSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequences[@type = 'opendds:WString']">
    <xsl:text>#include &lt;tao/WStringSeq.pidl&gt;
</xsl:text>
  </xsl:if>

  <!-- ecore don't know how to translate Enum 
       where from?  LongDoubleSeq.pidl 
  -->
    
</xsl:template>

</xsl:stylesheet>

