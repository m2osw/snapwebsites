<?xml version="1.0"?>
<!--
Snap Websites Server == password blacklist manager parser
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
	<xsl:variable name="layout-area">password-blacklist-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-09-07 23:22:10</xsl:variable>
	<xsl:variable name="layout-editor">password-blacklist-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form" form_name="blacklist">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>

				<h3>Password Blacklist Manager</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset class="query-database">
						<legend>Query Database</legend>

						<!-- check password widget -->
						<div class="editor-block">
							<label for="check_password" class="editor-title">Enter Password to Check:</label>
							<xsl:copy-of select="page/body/password/check_password/node()"/>
						</div>

						<div class="buttons">
							<a class="check-password button editor-action-button" href="#">Check Password</a>
						</div>

					</fieldset>

					<fieldset class="new-passwords">
						<legend>Add Passwords to Blacklist</legend>

						<!-- upload file widget -->
						<!-- we may add this one later, but really large file won't
						     make it so it is probably easier to just ask admins
								 to do that by hand directly on their cluster -->
						<!--div class="editor-block">
							<xsl:copy-of select="page/body/password/upload/node()"/>
						</div-->

						<!-- add passwords widget -->
						<div class="editor-block">
							<label for="check_password" class="editor-title">Enter Passwords to Add to Blacklist (EXACTLY ONE PASSWORD PER LINE):</label>
							<xsl:copy-of select="page/body/password/new_passwords/node()"/>
							<p class="small-warning">
								Note that passwords added in this way cannot really include
								spaces since HTML does not always manage spaces	as expected
								in a password. To add passwords with spaces, it is
								preferable to upload a file instead.
							</p>
						</div>

						<div class="buttons">
							<a class="add-passwords button editor-action-button" href="#">Add Passwords</a>
						</div>

					</fieldset>

					<fieldset class="old-passwords">
						<legend>Remove Passwords from Blacklist</legend>

						<!-- remove passwords widget -->
						<div class="editor-block">
							<label for="check_password" class="editor-title">Enter Passwords to Remove from Blacklist (EXACTLY ONE PASSWORD PER LINE):</label>
							<xsl:copy-of select="page/body/password/remove_passwords/node()"/>
						</div>

						<div class="buttons">
							<a class="remove-passwords button editor-action-button" href="#">Remove Passwords</a>
						</div>

					</fieldset>

					<!--div class="buttons">
						Add support for a delete in the parent instead?
						<a class="editor-save-button" href="#">Save Changes</a>
						<a class="editor-cancel-button center-aligned" href="{/snap/head/metadata/desc[@type='page_uri']/data}">Reset</a>
						<a class="editor-delete-button right-aligned" href="#">Delete</a>
					</div-->
				</div>
			</div>

			<javascript name="password-blacklist"/>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
