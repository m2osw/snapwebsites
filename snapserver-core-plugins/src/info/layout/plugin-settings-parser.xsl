<?xml version="1.0"?>
<!--
Snap Websites Server == plugin settings parser
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

	<!-- some special variables to define the theme -->
	<xsl:variable name="layout-area">plugin-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-12-18 16:22:10</xsl:variable>
	<xsl:variable name="layout-editor">plugin-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form" form_name="info">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h3>Snap! Plugins</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<p>List of available, default, and installed plugins.</p>

					<!-- plugin path widget -->
					<div class="editor-block">
						<label for="plugin_path" class="editor-title">Plugin Paths:</label>
						<xsl:copy-of select="page/body/plugin/path/node()"/>
					</div>

					<fieldset class="plugins">
						<legend>Plugins</legend>

						<!-- selection of plugins -->
						<xsl:for-each select="page/body/plugin/selection/*">
							<div class="editor-block">
								<xsl:copy-of select="./div/div/node()"/>
							</div>
						</xsl:for-each>

					</fieldset>

				</div>
			</div>

			<css name="info"/>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
