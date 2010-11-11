<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:lut='http://www.opendds.com/modeling/schemas/Lut/1.0'
     xmlns:opendds="http://www.opendds.org/modeling/schemas/OpenDDS/1.0"
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

<!-- Lookup table -->
<xsl:variable name="lut" select="document('lut.xml')/*/lut:types"/>

<!-- All types-->
<xsl:variable name="type"     select="/opendds:OpenDDSModel/dataLib/types"/>

<!-- Index (lookup table is in lut variable) -->
<xsl:key
     name  = "lut-type"
     match = "type"
     use   = "@type"/>

<!-- Name of the model -->
<xsl:variable name = "modelname" select = "/opendds:OpenDDSModel/dataLib/@name"/>

<!-- process the entire model document to produce the IDL. -->
<xsl:template match="/">

  <!-- required to build on windows -->
  <xsl:call-template name="processIntrinsicSequences"/>

  <xsl:value-of select="concat($newline,
                               'module ', $modelname, ' {', $newline,
                               '  // Forward declarations', $newline)"/>

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

  <xsl:value-of select="concat($newline, $newline, '};', $newline, $newline)"/>
</xsl:template>
<!-- End of main processing template. -->

<!-- Depth first traversal of type nodes processing predecessors first. -->
<xsl:template name="generate-idl">
  <xsl:param name="nodes"/>     <!-- <types> element nodes -->
  <xsl:param name="excluded"/>  <!-- Space separated string of type ids already processed. -->

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
    <xsl:variable name="direct-predecessors" select="$type[@xmi:id = current()//@type or @xmi:id = current()//@subtype or @xmi:id = current()//@switch]"/>

    <xsl:call-template name="generate-idl">
      <xsl:with-param name="nodes"
           select="$direct-predecessors[not(contains($exclude-list,concat(' ',@xmi:id,' ')))]"/>
      <xsl:with-param name="excluded" select="$exclude-list"/>
    </xsl:call-template>

    <!--
      ** Generate the IDL for this node after its predecessors
      ** have been processed, only if its predecessors did not include
      ** this node.
      -->
    <xsl:if test="not(contains($exclude-list,concat(' ',@xmi:id,' ')))">
      <xsl:apply-templates select="."/>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<!-- Depth first search for type names of predecessors. -->
<xsl:template name="get-dependencies">
  <!-- successors is the set of nodes already represented in the get-dependencies output.
       used to prevent duplicates. -->
  <xsl:param name="successors" select="/.."/>

  <xsl:variable name="direct-predecessors" select="$type[@xmi:id = current()//@type or @xmi:id = current()//@subtype or @xmi:id = current()//@switch]"/>

  <!-- using a Kaysian intersection predicate causes issues in eclipse here -->
  <xsl:for-each select="$direct-predecessors">
    <xsl:if test="not($successors[@xmi:id = current()/@xmi:id])">
      <xsl:call-template name="get-dependencies">
        <!-- Kaysian intersection are fine in eclipse without context switch -->
        <xsl:with-param name="successors" select=". | $successors"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:for-each>
  <xsl:text> </xsl:text>
  <xsl:value-of select="@xmi:id"/>
</xsl:template>

<!-- Forward declare union definitions. -->
<xsl:template match="types[@xsi:type = 'types:Union']" mode="declare">
  <xsl:value-of select="concat('  union ', @name, ';', $newline)"/>
</xsl:template>

<!-- Forward declare data structure definitions. -->
<xsl:template match="types[@xsi:type = 'types:Struct']" mode="declare">
  <xsl:value-of select="concat('  struct ', @name, ';', $newline)"/>
</xsl:template>

<!-- Ignore text in declare mode (enums) -->
<xsl:template match="text()" mode="declare">
</xsl:template>

<!-- Process enumeration definitions. -->
<xsl:template match="types[ @xsi:type = 'types:Enum']">
  <xsl:value-of select="concat('  enum ', @name, ' {', $newline)"/>

  <xsl:apply-templates select="literals" mode="enum"/>

  <xsl:value-of select="concat('  };', $newline)"/>
</xsl:template>

<!-- Process typedef definitions. -->
<xsl:template match="types[@xsi:type = 'types:Typedef']">
  <xsl:call-template name="define-type">
    <xsl:with-param name="targetid" select="@type"/>
    <xsl:with-param name="name" select="@name"/>
  </xsl:call-template>
</xsl:template>

<!-- Process union definitions. -->
<xsl:template match="types[@xsi:type = 'types:Union']">
  <xsl:value-of select="concat('  union ',@name,' switch (')"/>

  <xsl:call-template name="typename">
    <xsl:with-param name="target" select="$type[@xmi:id = current()/@switch]"/>
  </xsl:call-template>

  <xsl:value-of select="concat(') {', $newline)"/>

  <xsl:apply-templates select="branches"/>
  <xsl:apply-templates select="default"/>

  <xsl:value-of select="concat('  };', $newline)"/>
</xsl:template>

<!-- Process data structure definitions. -->
<xsl:template match="types[@xsi:type = 'types:Struct']">
  <xsl:value-of select="concat($newline,'#pragma DCPS_DATA_TYPE &quot;',
                               $modelname,'::', @name, '&quot;', $newline)"/>

  <xsl:apply-templates select="keys"/>

  <xsl:value-of select="concat('  struct ',@name,' {', $newline)"/>

  <xsl:apply-templates select="fields" mode="struct"/>

  <xsl:value-of select="concat('  };', $newline)"/>
</xsl:template>

<!-- Process individual union cases. -->
<xsl:template match="branches">
  <!-- handle mulitple cases for the variant... -->
  <xsl:for-each select="cases">
    <xsl:if test="position() > 1">
      <xsl:value-of select="$newline"/>
    </xsl:if>
    <xsl:value-of select="concat('    case ',@literal,': ')"/>
  </xsl:for-each>

  <xsl:variable name="typename">
    <xsl:call-template name="typename">
      <xsl:with-param name="target" select="$type[@xmi:id = current()/field/@type]"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- output variant ... -->
  <xsl:value-of select="concat($typename,' ',field/@name,';',$newline)"/>
</xsl:template>

<!-- Process union default. -->
<xsl:template match="default">
  <xsl:variable name="typename">
    <xsl:call-template name="typename">
      <xsl:with-param name="target" select="$type[@xmi:id = current()/@type]"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:value-of select="concat('    default: ',$typename,' ',@name,';',$newline)"/>
</xsl:template>

<!-- Process individual structure members. -->
<xsl:template match="fields" mode="struct">
  <!-- Build the output string for the type specification. -->
  <xsl:variable name="typename">
    <xsl:call-template name="typename">
      <xsl:with-param name="target" select="$type[@xmi:id = current()/@type]"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:value-of select="concat('    ',$typename,' ',@name,';',$newline)"/>
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
<xsl:template match="keys">
  <xsl:value-of select="concat('#pragma DCPS_DATA_KEY  &quot;',$modelname,
                        '::',../@name,' ',
                        ../fields[@xmi:id = current()/@field]/@name,
                        '&quot;',$newline)"/>
</xsl:template>

<xsl:template name="define-type">
  <xsl:param name="targetid"/>
  <xsl:param name="name"/>
  <xsl:text>  typedef </xsl:text>
  <xsl:call-template name="typename">
    <xsl:with-param name="target" select="$type[@xmi:id = $targetid]"/>
  </xsl:call-template>
  <xsl:value-of select="concat(' ',$name)"/>
  <xsl:call-template name="typesize">
    <xsl:with-param name="target" select="$type[@xmi:id = $targetid]"/>
  </xsl:call-template>
  <xsl:value-of select="concat(';',$newline)"/>
</xsl:template>

<xsl:template name="typename">
  <xsl:param name="target"/>
  <xsl:variable name="targetname" select="$target/@name"/>
  <xsl:variable name="targettype" select="$target/@xsi:type"/>

  <xsl:choose>
    <xsl:when test="string-length($targetname) > 0">
      <xsl:value-of select="$targetname"/>
    </xsl:when>
    <xsl:when test="string-length($targettype) > 0">
      <xsl:variable name="corbatype">
        <xsl:call-template name="corbatype">
          <xsl:with-param name="name" select="$targettype"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="$corbatype = 'array'">
          <xsl:call-template name="typename">
            <xsl:with-param name="target" select="$type[@xmi:id = $target/@subtype]"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$corbatype = 'sequence'">
          <xsl:text>sequence&lt;</xsl:text>
          <xsl:call-template name="typename">
            <xsl:with-param name="target" select="$type[@xmi:id = $target/@subtype]"/>
          </xsl:call-template>
          <xsl:if test="$target/@length">
            <xsl:value-of select="concat(', ',$target/@length)"/>
          </xsl:if>
          <xsl:text>&gt;</xsl:text>
        </xsl:when>
        <xsl:when test="$corbatype = 'string' or $corbatype = 'wstring'">
          <xsl:value-of select="$corbatype"/>
          <xsl:if test="$target/@length">
            <xsl:value-of select="concat('&lt;',$target/@length,'&gt;')"/>
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$corbatype"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>???</xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="typesize">
  <xsl:param name="target"/>
  <xsl:if test="$target/@xsi:type = 'types:Array'">
    <xsl:if test="$target/@length">
      <xsl:value-of select="concat('[',$target/@length,']')"/>
    </xsl:if>
  </xsl:if>
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

<xsl:template name="processIntrinsicSequences">

  <!-- pull tests to output each include only once -->
  <xsl:variable name="sequence-defs" select="$type[@xsi:type = 'types:Sequence']"/>
  <xsl:variable name="sequence-types" select="$type[@xmi:id = $sequence-defs/@subtype]"/>

  <xsl:if test="$sequence-types[@xsi:type = 'types:Boolean']">
    <xsl:text>#include &lt;tao/BooleanSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:Char']">
    <xsl:text>#include &lt;tao/CharSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:WChar']">
    <xsl:text>#include &lt;tao/WCharSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:Octet']">
    <xsl:text>#include &lt;tao/Octet.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:Double']">
    <xsl:text>#include &lt;tao/DoubleSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:Float']">
    <xsl:text>#include &lt;tao/FloatSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:Short']">
    <xsl:text>#include &lt;tao/ShortSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:Long' or @xsi:type = 'types:Integer']">
    <xsl:text>#include &lt;tao/LongSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:LongLong']">
    <xsl:text>#include &lt;tao/LongLongSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:UShort']">
    <xsl:text>#include &lt;tao/UShortSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:ULong' or @xsi:type = 'types:UInteger']">
    <xsl:text>#include &lt;tao/ULongSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:ULongLong']">
    <xsl:text>#include &lt;tao/ULongLongSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:String']">
    <xsl:text>#include &lt;tao/StringSeq.pidl&gt;
</xsl:text>
  </xsl:if>
  <xsl:if test="$sequence-types[@xsi:type = 'types:WString']">
    <xsl:text>#include &lt;tao/WStringSeq.pidl&gt;
</xsl:text>
  </xsl:if>

  <!-- ecore don't know how to translate Enum 
       where from?  LongDoubleSeq.pidl -->
    
</xsl:template>

</xsl:stylesheet>

