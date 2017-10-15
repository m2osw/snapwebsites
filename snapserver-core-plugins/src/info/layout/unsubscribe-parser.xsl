<?xml version="1.0"?>
<!--
Snap Websites Server == unsubscribe body layout setup
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
	<!--xsl:variable name="layout-name">unsubscribe</xsl:variable-->
	<xsl:variable name="layout-area">unsubscribe-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-05-25 15:28:01</xsl:variable>
	<xsl:variable name="layout-editor">unsubscribe-page</xsl:variable>

	<xsl:template match="snap">
		<output lang="{$lang}">
			<div class="editor-form" form_name="unsubscribe" mode="save-all">
				<xsl:attribute name="session"><xsl:value-of select="page/body/sendmail/form/unsubscribe/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h2>Email Subscriptions</h2>
				<div>
					<fieldset class="unsubscribe-settings">
						<legend>Settings</legend>

						<!-- location name, use page title -->
						<div class="editor-block">
							<label for="email" class="title">Email <span class="required">*</span></label>
							<xsl:copy-of select="page/body/sendmail/email/node()"/>
						</div>

						<div class="editor-block">
							<label for="level" class="title">Subscription Level <span class="required">*</span></label>
							<xsl:copy-of select="page/body/sendmail/level/node()"/>
						</div>

					</fieldset>
				</div>

				<div class="buttons">
					<a class="save-button editor-action-button" href="#">Save Preferences</a>
					<a class="cancel-button editor-action-button" href="/">Cancel</a>
				</div>

			</div>
			<div class="success" style="display: none;">
				<h2>Email Subscriptions</h2>
				<div>
					<!-- "next tab" == hidden by default -->
					<fieldset class="unsubscribe-thank-you">
						<legend>Thank You!</legend>
						<p>
							You have been succeessfully unsubscribed. To re-subscribed, you
							will either have to contact us or to create an account and
							control your subscription setup from there.
						</p>
					</fieldset>
				</div>

				<div class="buttons">
					<a class="unsubscribe-button editor-action-button" href="#">Unsubscribe Another Email</a>
				</div>
			</div>

			<javascript name="unsubscribe"/>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
