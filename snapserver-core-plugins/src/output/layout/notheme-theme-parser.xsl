<?xml version="1.0"?>
<!--
Snap Websites Server == notheme theme layout setup
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
	<!-- include system data -->
	<xsl:include href="functions"/>
	<xsl:include href="html-header"/>

	<!-- some special variables to define the theme -->
	<xsl:param name="layout-name">notheme</xsl:param>
	<xsl:param name="layout-area">notheme-theme-parser</xsl:param>
	<xsl:param name="layout-modified">2016-01-11 04:14:58</xsl:param>

	<xsl:template match="snap">
		<html>
			<xsl:call-template name="snap:html-attributes"/>
			<head>
				<xsl:call-template name="snap:html-header">
					<xsl:with-param name="theme-css" select="'/css/snap/notheme-style.css'"/>
				</xsl:call-template>
			</head>
			<xsl:apply-templates select="page/body"/>
		</html>
	</xsl:template>
	<xsl:template match="page/body">
		<body popup="popup">
			<xsl:call-template name="snap:body-attributes"/>
			<div class="page">
				<div class="header"><h1><xsl:choose>
						<xsl:when test="/snap/page/body/titles/long-title">
							<xsl:copy-of select="/snap/page/body/titles/long-title/node()"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:copy-of select="/snap/page/body/titles/title/node()"/>
						</xsl:otherwise>
					</xsl:choose></h1></div>
				<div><xsl:copy-of select="output/node()"/></div>
				<div style="clear: both;"></div>
			</div>
		</body>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
