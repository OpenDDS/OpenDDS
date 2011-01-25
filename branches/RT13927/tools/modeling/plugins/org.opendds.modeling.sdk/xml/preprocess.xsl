<xsl:stylesheet version='1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:xmi='http://www.omg.org/XMI'
     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'
     xmlns:opendds='http://www.opendds.org/modeling/schemas/OpenDDS/1.0'>
  <!--
    ** $Id$
    **
    ** Generate C++ header code.
    **
    -->
<xsl:include href="common.xsl"/>

<xsl:variable name="topics"      select="//topics"/>
<xsl:variable name="policy-refs" select="//*[not(name() = 'datatype')]/@href"/>

<!-- Copy the model element, adding external-refs subelelement -->
<xsl:template match="opendds:OpenDDSModel">
  <xsl:copy>
    <xsl:apply-templates select="@*"/>
    <xsl:apply-templates/>
    <external-refs>
      <xsl:call-template name="process-external-refs"/>
    </external-refs>
  </xsl:copy>
</xsl:template>

<!-- For any elemement with a child element with href attribute, 
     replace the href parent with a local reference to the element.
     For example:
         <abc>
           <def href="model.opendds#xyz"/>
         </abc>
     Becomes:
         <abc def="xyz"/>
-->
<xsl:template match="*[*[@href]]">
  <xsl:copy>
    <xsl:apply-templates select="@*"/>
    <xsl:for-each select="*[@href]">
      <xsl:call-template name="replace-href"/>
    </xsl:for-each>
    <xsl:apply-templates select="node()"/>
  </xsl:copy>
</xsl:template>

<!-- copy values -->
<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>

<!-- don't copy href attributes -->
<xsl:template match="@href">
</xsl:template>

<!-- href replacing logic.  Would be nice here if an 
     attribute could include a xpath expression.
-->
<xsl:template name="replace-href">
  <xsl:choose>
    <xsl:when test="name() = 'datatype'">
      <xsl:attribute name="datatype">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'type'">
      <xsl:attribute name="type">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'entity_factory'">
      <xsl:attribute name="entity_factory">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'user_data'">
      <xsl:attribute name="user_data">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'deadline'">
      <xsl:attribute name="deadline">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'destination_order'">
      <xsl:attribute name="destination_order">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'group_data'">
      <xsl:attribute name="group_data">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'presentation'">
      <xsl:attribute name="presentation">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'partition'">
      <xsl:attribute name="partition">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'durability'">
      <xsl:attribute name="durability">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'history'">
      <xsl:attribute name="history">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'latency_budget'">
      <xsl:attribute name="latency_budget">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'liveliness'">
      <xsl:attribute name="liveliness">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'ownership'">
      <xsl:attribute name="ownership">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'reliability'">
      <xsl:attribute name="reliability">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'resource_limits'">
      <xsl:attribute name="resource_limits">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'durability_service'">
      <xsl:attribute name="durability_service">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'ownership_strength'">
      <xsl:attribute name="ownership_strength">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'transport_priority'">
      <xsl:attribute name="transport_priority">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'lifespan'">
      <xsl:attribute name="lifespan">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'reader_data_lifecycle'">
      <xsl:attribute name="reader_data_lifecycle">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'time_based_filter'">
      <xsl:attribute name="time_based_filter">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>
    <xsl:when test="name() = 'topic_data'">
      <xsl:attribute name="topic_data">
        <xsl:value-of select="substring-after(@href, '#')"/>
      </xsl:attribute>
    </xsl:when>

    <xsl:otherwise>
      <xsl:message>Unknown href container element <xsl:value-of select="name()"/></xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- Copy external elements into the result set, with the following
     changes:
     1) make name a qualified name
     2) add the name of model in a new model attribute.
-->
<xsl:template name="process-external-refs">
  <xsl:param name="refs" select="//@href"/>
  <xsl:param name="complete-refs" select="' '"/>
  
  <xsl:if test="$refs">
    <xsl:variable name="ref" select="$refs[1]"/>
    <xsl:variable name="model" select="substring-before($ref, '#')"/>
    <xsl:variable name="id" select="substring-after($ref, '#')"/>
    <xsl:if test="not(contains($complete-refs, $id))">
      <xsl:for-each select="document($model)//*[@xmi:id = $id]">
        <xsl:copy>
          <xsl:attribute name="model">
            <xsl:call-template name="modelname"/>
          </xsl:attribute>
          <xsl:if test="@name">
            <xsl:variable name="scopename">
              <xsl:call-template name="scopename"/>
            </xsl:variable>
            <xsl:attribute name="name">
              <xsl:value-of select="concat($scopename, @name)"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:apply-templates select="@*[name() != 'name'] | node()"/>
        </xsl:copy>
      </xsl:for-each>
    </xsl:if>
    <xsl:call-template name="process-external-refs">
      <xsl:with-param name="refs" select="$refs[position() &gt; 1]"/>
      <xsl:with-param name="complete-refs" select="concat(' ', $id, ' ')"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!-- Determine the name of a model -->
<xsl:template name="modelname">
  <xsl:param name="target" select="."/>
  <xsl:choose>
    <xsl:when test="name($target) = 'opendds:OpenDDSModel'">
      <xsl:value-of select="$target/@name"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="modelname">
        <xsl:with-param name="target" select="$target/.."/>
      </xsl:call-template>

    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
