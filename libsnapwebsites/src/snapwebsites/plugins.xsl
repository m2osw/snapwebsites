<?xml version="1.0"?>
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- Match on Attributes, Elements, text nodes, and Processing Instructions -->
  <xsl:template match="@*| * | text() | processing-instruction()">
     <xsl:copy>
        <xsl:apply-templates select="@*|node()"/>
     </xsl:copy>
  </xsl:template>

  <!-- Empty template prevents comments from being copied into the output -->
  <xsl:template match="comment()"/>

</xsl:stylesheet>
