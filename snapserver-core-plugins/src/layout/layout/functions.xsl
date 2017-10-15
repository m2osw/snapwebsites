<?xml version="1.0"?>
<!--
Snap Websites Server == system messages XSLT templates
Copyright (C) 2011-2017  Made to Order Software Corp.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
-->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
															xmlns:xs="http://www.w3.org/2001/XMLSchema"
															xmlns:fn="http://www.w3.org/2005/xpath-functions"
															xmlns:snap="snap:snap">

	<!-- *** SYSTEM PARAMETERS *** -->

  <xsl:param name="use_dcterms">yes</xsl:param>

	<!-- *** SYSTEM VARIABLES *** -->

  <!-- get the website URI (i.e. URI without any folder other than the website base folder) -->
  <xsl:variable name="website_uri" as="xs:string" select="/snap/head/metadata/desc[@type='website_uri']/data"/>

  <!-- compute the protocol from the main URI -->
  <xsl:variable name="protocol" as="xs:string">
    <xsl:value-of select="substring-before($website_uri, '://')"/>
  </xsl:variable>

  <!-- compute the domain from the main URI -->
  <xsl:variable name="domain" as="xs:string">
    <xsl:choose>
      <xsl:when test="contains($website_uri, '://www.')">
        <xsl:value-of select="substring-before(substring-after($website_uri, '://www.'), '/')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before(substring-after($website_uri, '://'), '/')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- get the base URI (i.e. parent URI to this page) -->
  <xsl:variable name="base_uri" as="xs:string" select="/snap/head/metadata/desc[@type='base_uri']/data"/>

  <!-- compute the path from the main URI to this page -->
  <xsl:variable name="path" as="xs:string">
    <xsl:value-of select="substring-after($base_uri, $website_uri)"/>
  </xsl:variable>

  <!-- get the page URI (i.e. the protocol, domain name, and full path to this page) -->
  <xsl:variable name="page_uri" as="xs:string" select="/snap/head/metadata/desc[@type='page_uri']/data"/>

  <!-- compute the full path from the main URI to this page -->
  <xsl:variable name="full_path" as="xs:string">
    <xsl:value-of select="substring-after(substring-after($page_uri, '://'), '/')"/>
  </xsl:variable>

  <!-- compute the relative path from the root of this website -->
  <xsl:variable name="page" as="xs:string">
    <xsl:value-of select="substring-after($page_uri, $base_uri)"/>
  </xsl:variable>

  <!-- get the action -->
  <xsl:variable name="action" as="xs:string" select="/snap/head/metadata/desc[@type='action']/data"/>

  <!-- get the can_edit -->
  <xsl:variable name="can_edit" as="xs:string" select="/snap/head/metadata/desc[@type='can_edit']/data"/>

  <!-- get the year the page was created -->
  <xsl:variable name="year" select="year-from-date(current-date())"/>
  <xsl:variable name="created_year" select="year-from-date(/snap/page/body/created)"/>
  <xsl:variable name="year_range">
		<xsl:value-of select="$created_year"/><xsl:if test="$created_year != xs:integer($year)">-<xsl:value-of select="$year"/></xsl:if>
  </xsl:variable>

  <!-- get the page language -->
  <xsl:variable name="lang">
    <xsl:choose><!-- make sure we get some language, we default to English -->
      <xsl:when test="/snap/page/body/lang"><xsl:value-of select="/snap/page/body/lang"/></xsl:when>
      <xsl:otherwise>en</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

	<!-- *** SYSTEM FUNCTIONS *** -->

  <!-- transform the specified path in a full path as required -->
  <xsl:function name="snap:prepend-base" as="xs:string">
    <xsl:param name="website_uri" as="xs:string"/>
    <xsl:param name="base_uri" as="xs:string"/>
    <xsl:param name="path" as="xs:string"/>
    <xsl:variable name="lpath"><xsl:value-of select="lower-case($path)"/></xsl:variable>

    <xsl:choose>
      <xsl:when test="matches($lpath, '^[a-z]+://')">
        <!-- full path, use as is -->
        <xsl:value-of select="$path"/>
      </xsl:when>
      <xsl:when test="starts-with($lpath, '/')">
        <!-- root path, use with website URI -->
        <xsl:value-of select="concat($website_uri, substring($path, 2))"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- relative path, use with base URI -->
        <xsl:value-of select="concat($base_uri, $path)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:function>

  <xsl:template name="snap:html-attributes">
		<xsl:attribute name="lang"><xsl:value-of select="$lang"/></xsl:attribute>
		<xsl:attribute name="xml:lang"><xsl:value-of select="$lang"/></xsl:attribute>
		<xsl:attribute name="prefix">og: http://ogp.me/ns#</xsl:attribute>
		<!-- the following includes an extra class, "snap", so the two names
		     (layout-name and theme-name) are spaced out; Qt concatenate them
				 otherwise... -->
		<xsl:attribute name="class"><xsl:value-of
				select="/snap/head/metadata/@layout-name"/> snap <xsl:value-of
				select="/snap/head/metadata/@theme-name"/> standard <xsl:value-of
				select="substring-after(/snap/head/metadata/desc[@type='login_status']/data, 'status::')"/> no-js</xsl:attribute>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
