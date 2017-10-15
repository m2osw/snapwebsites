<?xml version="1.0"?>
<!--
Snap Websites Server == info settings parser
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
	<xsl:variable name="layout-area">info-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-09-07 23:22:10</xsl:variable>
	<xsl:variable name="layout-editor">info-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form" form_name="info">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<!-- xsl:if test="$action != 'edit' and $can_edit = 'yes'">
					<a class="settings-edit-button" href="?a=edit">Edit</a>
				</xsl:if>
				<xsl:if test="$action = 'edit'">
					<a class="settings-save-button" href="#">Save Changes</a>
					<a class="settings-cancel-button right-aligned" href="{/snap/head/metadata/desc[@type='page_uri']/data}">Cancel</a>
				</xsl:if-->
				<h3>Basic Website Information</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<p>You are running Snap! v[version] (more <a href="/admin/versions?back=admin/settings/info">version information</a>)</p>
					<p>Site wide settings as offerred by the core system of your website.</p>

					<fieldset class="site-name">
						<legend>Site Name</legend>

						<!-- site name widget -->
						<div class="editor-block">
							<label for="site_name_widget" class="editor-title">Site Name:</label>
							<xsl:copy-of select="page/body/info/site_name/node()"/>
						</div>

						<!-- site long name widget -->
						<div class="editor-block">
							<label for="site_long_name_widget" class="editor-title">Site Long Name:</label>
							<xsl:copy-of select="page/body/info/site_long_name/node()"/>
						</div>

						<!-- site short name widget -->
						<div class="editor-block">
							<label for="site_short_name_widget" class="editor-title">Site Short Name:</label>
							<xsl:copy-of select="page/body/info/site_short_name/node()"/>
						</div>
					</fieldset>

					<fieldset class="administrator-email">
						<legend>Administrator Email</legend>

						<!-- administrator email -->
						<div class="editor-block">
							<label for="administrator_email" class="editor-title">Administrator Email:</label>
							<xsl:copy-of select="page/body/info/administrator_email/node()"/>
						</div>
					</fieldset>

					<fieldset class="breadcrumbs">
						<legend>Breadcrumbs</legend>

						<!-- breadcrumb home label widget -->
						<div class="editor-block">
							<label for="site_name_widget" class="editor-title">Home Label:</label>
							<xsl:copy-of select="page/body/info/breadcrumbs_home_label/node()"/>
						</div>

						<!-- show home widget -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/info/breadcrumbs/show_home/node()"/>
						</div>

						<!-- show current page widget -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/info/breadcrumbs/show_current_page/node()"/>
						</div>
					</fieldset>

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
