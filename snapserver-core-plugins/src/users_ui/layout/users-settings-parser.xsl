<?xml version="1.0"?>
<!--
Snap Websites Server == users settings parser
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
	<xsl:variable name="layout-area">users-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-12-13 18:19:53</xsl:variable>
	<xsl:variable name="layout-editor">users-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output><!-- lang="{$lang}"-->
			<div class="editor-form" form_name="info">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h3>Users Settings</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset class="site-name">
						<legend>Session Information</legend>

						<!-- soft administration session widget -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/users/soft_administrative_session/node()"/>
						</div>

						<!-- administration session duration widget -->
						<div class="editor-block">
							<label for="administrative_session_duration" class="editor-title">Administration Session Duration (in minutes):</label>
							<xsl:copy-of select="page/body/users/administrative_session_duration/node()"/>
						</div>

						<!-- user session duration widget -->
						<div class="editor-block">
							<label for="user_session_duration" class="editor-title">User Session Duration (in minutes):</label>
							<xsl:copy-of select="page/body/users/user_session_duration/node()"/>
						</div>

						<!-- total session duration widget -->
						<div class="editor-block">
							<label for="total_session_duration" class="editor-title">Total Session Duration (in minutes):</label>
							<xsl:copy-of select="page/body/users/total_session_duration/node()"/>
						</div>

					</fieldset>

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
