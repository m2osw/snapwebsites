<?xml version="1.0"?>
<!--
Snap Websites Server == flash setting page data to XML
Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved

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
	<xsl:variable name="layout-area">flash-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-11-14 18:14:57</xsl:variable>
	<xsl:variable name="layout-editor">flash-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output><!-- lang="{$lang}"-->
			<div class="editor-form" form_name="flash">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h2>Flash Settings</h2>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset>
						<legend>Basic Settings</legend>

						<div class="editor-block">
							<xsl:copy-of select="page/body/flash/enabled/node()"/>
						</div>
					</fieldset>

				</div>
			</div>

		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
