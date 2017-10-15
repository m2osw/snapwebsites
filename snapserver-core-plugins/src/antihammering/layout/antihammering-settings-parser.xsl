<?xml version="1.0"?>
<!--
Snap Websites Server == antihammering settings parser
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
	<xsl:variable name="layout-area">antihammering-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-12-27 04:47:26</xsl:variable>
	<xsl:variable name="layout-editor">antihammering-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form" form_name="password">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h3>Antihammering Settings</h3>
				<div>

					<fieldset class="site-name">
						<legend>Fast Valid Accesses</legend>

						<div class="editor-block">
							<label for="hit_limit" class="editor-title">Hit Limit:</label>
							<xsl:copy-of select="page/body/antihammering/hit_limit/node()"/>
						</div>

						<div class="editor-block">
							<label for="hit_limit_duration" class="editor-title">Hit Limit Duration (seconds):</label>
							<xsl:copy-of select="page/body/antihammering/hit_limit_duration/node()"/>
						</div>

						<div class="editor-block">
							<label for="first_pause" class="editor-title">First Pause:</label>
							<xsl:copy-of select="page/body/antihammering/first_pause/node()"/>
						</div>

						<div class="editor-block">
							<label for="second_pause" class="editor-title">Second Pause:</label>
							<xsl:copy-of select="page/body/antihammering/second_pause/node()"/>
						</div>

						<div class="editor-block">
							<label for="third_pause" class="editor-title">Third Pause:</label>
							<xsl:copy-of select="page/body/antihammering/third_pause/node()"/>
						</div>

					</fieldset>

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
