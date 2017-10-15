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
	<!-- include system data -->
	<xsl:include href="functions"/>

	<!-- some special variables to define the theme -->
	<xsl:variable name="layout-name">snap_software_description</xsl:variable>
	<xsl:variable name="layout-area">snap-software-description-parser</xsl:variable>
	<xsl:variable name="layout-modified">2014-11-29 02:35:36</xsl:variable>

	<!-- handle the icon link -->
	<xsl:template name="shortcut-link">
		<xsl:param name="type"/>
		<xsl:param name="href"/>
		<xsl:param name="width"/>
		<xsl:param name="height"/>
		<!-- TBD: support other size if no 32x32 is available? -->
		<xsl:if test="$width = 32 and $height = 32">
			<icon width="{$width}" height="{$height}"><xsl:value-of select="$href"/></icon>
		</xsl:if>
	</xsl:template>

	<xsl:template match="snap">
		<output lang="{$lang}">
			<snsd-file rest="{$base_uri}snap-software-description" self="{$page_uri}/file.xml">
        <!-- moved version in an XSL attribute to avoid XSLT v1.0 errors -->
        <xsl:attribute name="version">1.0</xsl:attribute>
				<xsl:if test="page/body/publisher">
					<xsl:attribute name="publisher"><xsl:value-of select="page/body/publisher/node()"/></xsl:attribute>
				</xsl:if>
				<xsl:if test="page/body/support">
					<xsl:attribute name="support"><xsl:value-of select="page/body/support/node()"/></xsl:attribute>
				</xsl:if>
				<generator>Snap! Websites v<xsl:value-of select="head/metadata/desc[@type='version']/data"/></generator>
				<specification>http://snapwebsites.org/implementation/feature-requirements/pad-and-snsd-files-feature/snap-software-description</specification>
				<technical-name><xsl:value-of select="page/body/titles/title/node()"/></technical-name>
				<link><xsl:value-of select="$page_uri"/></link>
				<description>
					<!-- TODO: add support for languages... -->
					<name><xsl:value-of select="page/body/titles/title/node()"/></name>

					<!-- search shortcut icons for a 32x32 size one -->
					<xsl:for-each select="page/body/image/shortcut">
						<xsl:call-template name="shortcut-link">
							<xsl:with-param name="type" select="@type"/>
							<xsl:with-param name="href" select="@href"/>
							<xsl:with-param name="width" select="@width"/>
							<xsl:with-param name="height" select="@height"/>
						</xsl:call-template>
					</xsl:for-each>

					<!-- TODO: verify that the size fits -->
					<xsl:if test="page/body/titles/short-title">
						<desc45><xsl:value-of select="page/body/titles/short-title/node()"/></desc45>
					</xsl:if>
					<xsl:if test="page/body/titles/title">
						<desc80><xsl:value-of select="page/body/titles/title/node()"/></desc80>
					</xsl:if>
					<xsl:if test="page/body/titles/long-title">
						<desc250><xsl:value-of select="page/body/titles/long-title/node()"/></desc250>
					</xsl:if>
					<xsl:if test="page/body/abstract">
						<desc450><xsl:value-of select="page/body/abstract/node()"/></desc450>
					</xsl:if>
					<xsl:if test="page/body/description">
						<desc2000><xsl:value-of select="page/body/description/node()"/></desc2000>
					</xsl:if>
					<xsl:if test="page/body/description">
						<desc><xsl:copy-of select="page/body/output/*"/></desc>
					</xsl:if>

					<!-- TBD: assume we can add screenshots as required in SNSD files -->
					<xsl:copy-of select="page/body/screenshots/*"/>

				</description>
				<category>
					<class></class>
					<keywords></keywords>
				</category>
				<compatibility>
					<os>
						<version></version>
					</os>
					<language></language>
					<requirements></requirements>
				</compatibility>
				<license>
					<abstract></abstract>
					<verbatim></verbatim>
					<costs></costs>
					<expire></expire>
					<distribution-permissions/>
				</license>
				<release>
					<download>
						<link></link>
						<size></size>
					</download>
					<changelog></changelog>
				</release>
			</snsd-file>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
