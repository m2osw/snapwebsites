<?xml version="1.0"?>
<!--
Snap Websites Server == no iframe settings parser
Copyright (C) 2017  Made to Order Software Corp.

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
	<xsl:variable name="layout-area">no_iframe-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2017-01-31 21:21:10</xsl:variable>
	<xsl:variable name="layout-editor">no_iframe-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form" form_name="info">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h3>No IFrame Settings</h3>
				<div>
					<p>Select the mode below. The default is "Always" which means none of
					your website pages can appear in an IFrame. At a later time, we will
					add the necessary to properly support popups.</p>

					<fieldset class="no-iframe-mode">
						<legend>No IFrame Mode</legend>

						<!-- site name widget -->
						<div class="editor-block">
							<label for="mode" class="editor-title">Mode:</label>
							<xsl:copy-of select="page/body/no_iframe/mode/node()"/>
						</div>

					</fieldset>

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
