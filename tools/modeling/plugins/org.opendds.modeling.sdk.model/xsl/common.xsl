<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
     xmlns:xmi='http://www.omg.org/XMI'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'>
  <!--
    **
    ** Common stylesheet templates and variables
    **
    -->
<xsl:variable name="lower" select="'abcdefghijklmnopqrstuvwxyz'"/>
<xsl:variable name="upper" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>

<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

<!--
  Given a target, will return A::B::C::D::name using ancestor 
  package and DCPS lib names.
-->
<xsl:template name="scopename">
  <xsl:param name="target" select="."/>

  <xsl:choose>
    <xsl:when test="not($target)">
    </xsl:when>
    <xsl:when test="name($target) = 'opendds:OpenDDSModel'">
    </xsl:when>
    <xsl:when test="name($target) = 'external-refs'">
      <!-- Scope must included in ref -->
    </xsl:when>
    <xsl:when test="$target/@scope">
      <xsl:value-of select="$target/@scope"/>
    </xsl:when>
    <!-- for external topics -->
    <xsl:when test="name($target) = 'libs' and $target/@xsi:type='opendds:DcpsLib'">
      <xsl:variable name="prefix">
        <xsl:call-template name="scopename">
          <xsl:with-param name="target" select="$target/.."/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat($prefix, $target/@name, '::')"/>
    </xsl:when>
    <xsl:when test="name($target) = 'libs'">
      <xsl:call-template name="scopename">
        <xsl:with-param name="target" select="$target/.."/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="name($target) = 'packages'">
      <xsl:variable name="prefix">
        <xsl:call-template name="scopename">
          <xsl:with-param name="target" select="$target/.."/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat($prefix, $target/@name, '::')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="scopename">
        <xsl:with-param name="target" select="$target/.."/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Normalize an identifier, taking out invalid chars -->
<xsl:template name="normalize-identifier">
  <xsl:param name="identifier" select="@name"/>
  <xsl:value-of select="translate($identifier, ' -:', '___')"/>
</xsl:template>

<!-- Normalize an type enumeration identifier -->
<xsl:template name="type-enum">
  <xsl:param name="type" select="."/>
  <xsl:call-template name="normalize-identifier">
    <xsl:with-param name="identifier" 
                    select="concat($type/@scope, $type/@name)"/>
  </xsl:call-template>
</xsl:template>

<!-- Normalize a topic enumeration identifier -->
<xsl:template name="topic-enum">
  <xsl:param name="topic" select="."/>
  <xsl:variable name="topic-scope">
    <xsl:call-template name="scopename">
      <xsl:with-param name="target" select="$topic"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:call-template name="normalize-identifier">
    <xsl:with-param name="identifier" 
                    select="concat($topic-scope, $topic/@name)"/>
  </xsl:call-template>
</xsl:template>

<!-- Capitalize the first letter of a value -->
<xsl:template name="capitalize">
  <xsl:param name="value"/>
  <xsl:variable name="first" select="substring($value, 1, 1)"/>
  <xsl:variable name="FIRST" select="translate($first, $lower, $upper)"/>
  <xsl:variable name="rest" select="substring($value, 2)"/>
  <xsl:value-of select="concat($FIRST, $rest)"/>
</xsl:template>

<!-- Utility to replace all occurances of parameter "replace"
     in "text" with "by" -->
 <xsl:template name="string-replace-all">
    <xsl:param name="text" />
    <xsl:param name="replace" />
    <xsl:param name="by" />
    <xsl:choose>
      <xsl:when test="contains($text, $replace)">
        <xsl:value-of select="substring-before($text,$replace)" />
        <xsl:value-of select="$by" />
        <xsl:call-template name="string-replace-all">
          <xsl:with-param name="text"
          select="substring-after($text,$replace)" />
          <xsl:with-param name="replace" select="$replace" />
          <xsl:with-param name="by" select="$by" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

<!-- Determine the qualified name of a target element.
     Different from scopename in that it has OpenDDS::Model at the 
     outermost layer.  This is used for referencing the Elements
     class defined in *_T.h
 -->
<xsl:template name="elements-qname">
  <xsl:param name="target" select="."/>
  <xsl:param name="previous" select="''"/>

  <xsl:choose>
    <xsl:when test="name($target) = 'opendds:OpenDDSModel'">
      <xsl:value-of select="concat('OpenDDS::Model::', $previous, '::Elements')"/>
    </xsl:when>
    <xsl:when test="name($target) = 'libs' and 
                    $target[@xsi:type = 'opendds:DcpsLib']">
      <xsl:call-template name="elements-qname">
        <xsl:with-param name="target" select="$target/.."/>
        <xsl:with-param name="previous" select="$target/@name"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="name($target) = 'packages'">
      <xsl:call-template name="elements-qname">
        <xsl:with-param name="target" select="$target/.."/>
        <xsl:with-param name="previous" select="concat($target/@name, '::', $previous)"/>
      </xsl:call-template>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<!-- Determine the qualified name of the Elements::Data class -->
<xsl:template name="data-qname">
  <xsl:call-template name="elements-qname"/>
  <xsl:text>::Data</xsl:text>
</xsl:template>

<!-- Determine the names of models referenced by this model. -->
<xsl:template name="data-model-ref-names">
  <xsl:param name="model-refs"/>
  <xsl:param name="complete-refs" select="''"/>

  <xsl:if test="$model-refs">
    <xsl:variable name="model-ref" select="$model-refs[1]"/>
    <xsl:variable name="model-file" select="substring-before($model-ref, '#')"/>

    <xsl:if test="not(contains($complete-refs, $model-file))">
      <xsl:if test="string-length($complete-refs) &gt; 0">
        <xsl:text>, </xsl:text>
      </xsl:if>
      <xsl:value-of select="document($model-file)/opendds:OpenDDSModel/@name"/>
    </xsl:if>

    <xsl:call-template name="data-model-ref-names">
      <xsl:with-param name="model-refs" select="$model-refs[position() &gt; 1]"/>
      <xsl:with-param name="complete-refs" select="concat($complete-refs, ' ', $model-file)"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template name="output-comment">
  <xsl:param name="comment" select="comment"/>
  <xsl:param name="indent" select="''"/>

  <xsl:choose>
    <xsl:when test="not($comment)">
    </xsl:when>
    <xsl:when test="contains($comment/@body, $newline)">
      <xsl:call-template name="output-multi-line-comment">
        <xsl:with-param name="comment" select="$comment"/>
        <xsl:with-param name="indent" select="$indent"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="output-single-line-comment">
        <xsl:with-param name="comment" select="$comment"/>
        <xsl:with-param name="indent" select="$indent"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="output-single-line-comment">
  <xsl:param name="comment" select="comment"/>
  <xsl:param name="indent" select="''"/>
  <xsl:variable name="prefix">
    <xsl:choose>
      <xsl:when test="$comment/@format='PLAIN'">
        <xsl:value-of select="'//  '"/>
      </xsl:when>
      <xsl:when test="$comment/@format='DOXYGEN'">
        <xsl:value-of select="'/// '"/>
      </xsl:when>
    </xsl:choose>
  </xsl:variable>

  <xsl:value-of select="concat($indent, $prefix, $comment/@body, $newline)"/>
</xsl:template>

<xsl:template name="output-multi-line-comment-line">
  <xsl:param name="value"/>
  <xsl:param name="indent" select="''"/>

  <xsl:variable name="line" select="substring-before($value, $newline)"/>

  <!-- 
       We could have the following cases for value:
      
       1) text <newline> text
       2) text
       3) text <newline>
       4) <newline> text
       5) <empty>
    -->
  <xsl:choose>
    <xsl:when test="not($value)"/>
    <xsl:when test="not(contains($value, $newline))">
      <!-- on the last line, there is no newline -->
      <xsl:value-of select="concat($indent, ' * ', $value, $newline)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat($indent, ' * ', substring-before($value, $newline), $newline)"/>
      <xsl:call-template name="output-multi-line-comment-line">
        <xsl:with-param name="value" select="substring-after($value, $newline)"/>
        <xsl:with-param name="indent" select="$indent"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="output-multi-line-comment">
  <xsl:param name="comment" select="comment"/>
  <xsl:param name="indent" select="''"/>
  <xsl:choose>
    <xsl:when test="$comment/@format='PLAIN'">
      <xsl:value-of select="concat($indent, '/*', $newline)"/>
    </xsl:when>
    <xsl:when test="$comment/@format='DOXYGEN'">
      <xsl:value-of select="concat($indent, '/**', $newline)"/>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="output-multi-line-comment-line">
    <xsl:with-param name="value" select="$comment/@body"/>
    <xsl:with-param name="indent" select="$indent"/>
  </xsl:call-template>
  <xsl:value-of select="concat($indent, ' */', $newline)"/>
</xsl:template>

<xsl:template name="substring-before-last">
  <xsl:param name="value"/>
  <xsl:param name="to-find"/>

  <xsl:variable name="to-find-length" select="string-length($to-find)"/>
  <xsl:variable name="value-length" select="string-length($value)"/>
  <xsl:choose>
    <xsl:when test="$value-length = 0"></xsl:when>
    <xsl:when test="substring($value, ($value-length + 1 - $to-find-length), $to-find-length) = $to-find">
      <xsl:value-of select="substring($value, 1, string-length($value) - string-length($to-find))"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="substring-before-last">
        <xsl:with-param name="value" select="substring($value, 1, string-length($value) - 1)"/>
        <xsl:with-param name="to-find" select="$to-find"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>

