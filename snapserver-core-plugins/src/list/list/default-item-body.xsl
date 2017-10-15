<?xml version="1.0"?>
<!--
Snap Websites Server == default item body parser
Copyright (C) 2014-2017  Made to Order Software Corp.

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
	<xsl:param name="layout-name">default-item</xsl:param>
	<xsl:param name="layout-area">default-item-body</xsl:param>
	<xsl:param name="layout-modified">2014-02-19 18:00:22</xsl:param>

	<!-- get the base URI (i.e. parent URI to this page) -->
	<xsl:variable name="real_uri" as="xs:string" select="/snap/head/metadata/desc[@type='real_uri']/data"/>
	<!-- compute the full path from the main URI to this page -->
	<xsl:variable name="full_path" as="xs:string" select="substring-after(substring-after($real_uri, '://'), '/')"/>

	<xsl:template match="snap">
		<output>
			<li>
				<xsl:attribute name="class">list-item index<xsl:value-of select="@index"/><xsl:if test="@index mod 2 = 0"> even</xsl:if><xsl:if test="@index mod 2 = 1"> odd</xsl:if></xsl:attribute>
				<a href="/{$full_path}">
					<xsl:attribute name="class">list-item-anchor
						<xsl:if test="/snap/head/metadata/desc[@type='main_page_path']/data = $full_path"> active</xsl:if>
					</xsl:attribute>
					<xsl:if test="page/body/titles/long-title">
						<!-- description of link makes use of long title if available -->
						<!-- TODO: give the description [once available] priority over the long-title (the one for the meta data...) -->
						<xsl:attribute name="title"><xsl:copy-of select="page/body/titles/long-title/node()"/></xsl:attribute>
					</xsl:if>
					<xsl:copy-of select="page/body/titles/title/node()"/>
				</a>
			</li>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
