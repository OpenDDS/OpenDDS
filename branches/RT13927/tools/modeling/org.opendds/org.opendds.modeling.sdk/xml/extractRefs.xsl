<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="xml" />

	<xsl:template match="/">
		<xsl:text>
</xsl:text>
		<xsl:variable name="refs" select="//@href"/>
		<xsl:for-each select="$refs">
			<xsl:variable name="curpos" select="position()"/>
			<xsl:variable name="priors" select="$refs[ position() &lt; $curpos]"/>
			<xsl:if test="count($priors[contains(.,substring-before(current(),'#'))]) = 0">
				<!-- Process references the first time we see one for a new file. -->
				<xsl:apply-templates select="."/>
			</xsl:if>
		</xsl:for-each>
	</xsl:template>
	
	<!-- Actually format and send a dependency result. -->
	<xsl:template match="@href">
		<xsl:element name="dependency">
			<xsl:attribute name="file">
            	<xsl:value-of select="substring-before(.,'#')" />
   			</xsl:attribute>
		</xsl:element>
		<xsl:text>
</xsl:text>
	</xsl:template>

</xsl:stylesheet>