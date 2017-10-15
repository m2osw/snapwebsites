<?xml version="1.0"?>
<!--
  This stylesheet works fine with Qt (xmlpatterns)
  but not with libxml2 (xslproc)
-->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                              xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
<xsl:template match="urlset"><xsl:apply-templates/></xsl:template>
<xsl:template match="url"><xsl:value-of select="loc"/><xsl:text xml:space="preserve">
</xsl:text></xsl:template>
</xsl:stylesheet>
