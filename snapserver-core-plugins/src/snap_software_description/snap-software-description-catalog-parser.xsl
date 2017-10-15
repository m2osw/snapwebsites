<?xml version="1.0"?>
<!--
Snap Websites Server == Snap Software Description page data to XML
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

	<!-- some special variables to define the theme -->
	<xsl:variable name="layout-name">snap_software_description</xsl:variable>
	<xsl:variable name="layout-area">snap-software-description-parser</xsl:variable>
	<xsl:variable name="layout-modified">2014-11-29 02:35:36</xsl:variable>

	<xsl:template match="file">
		<file><xsl:value-of select="."/>/file.xml</file>
	</xsl:template>

	<xsl:template match="sub-category">
		<catalog><xsl:attribute name="category"><xsl:value-of select="@name"/></xsl:attribute><xsl:value-of select="."/>/catalog.xml</catalog>
	</xsl:template>

	<xsl:template match="snap">
		<snsd-catalog rest="{base_uri/node()}snap-software-description">
      <!-- moved version in an XSL attribute to avoid XSLT v1.0 errors -->
      <xsl:attribute name="version">1.0</xsl:attribute>
			<xsl:attribute name="self"><xsl:value-of select="page_uri/node()"/><xsl:choose>
				<xsl:when test="ends-with(page_uri/node(), '/category')">/snap-software-description.xml</xsl:when>
				<xsl:otherwise>/catalog.xml</xsl:otherwise>
				</xsl:choose></xsl:attribute>
			<generator>Snap! Websites</generator>
			<specification>http://snapwebsites.org/implementation/feature-requirements/pad-and-snsd-files-feature/snap-software-description</specification>
			<xsl:if test="toc">
				<toc><xsl:value-of select="toc/node()"/></toc>
			</xsl:if>
			<xsl:apply-templates select="file"/>
			<xsl:apply-templates select="catalog"/>
		</snsd-catalog>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
