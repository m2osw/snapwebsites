<?xml version="1.0"?>
<!--
Snap Websites Server == editor cooke consent silktide parser for our form
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
	<xsl:variable name="layout-area">cookie-consent-silktide-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-08-18 15:39:02</xsl:variable>
	<xsl:variable name="layout-editor">cookie-consent-silktide-page</xsl:variable>

	<xsl:template match="snap">
		<output><!-- lang="{$lang}"-->
			<div class="editor-form" form_name="locale_timezone">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h3>Setup the Cookie Consent silktide</h3>
				<div class="settings">

					<fieldset class="cookie-consent-message">
						<legend>Message</legend>

						<!-- message widget -->
						<div class="editor-block">
							<label for="message" class="editor-title">Message: <span class="required">*</span></label>
							<xsl:copy-of select="page/body/cookie_consent_silktide/message/node()"/>
						</div>

						<!-- learn more link label -->
						<div class="editor-block">
							<label for="message" class="editor-title">Learn More Label:</label>
							<xsl:copy-of select="page/body/cookie_consent_silktide/learn_more_label/node()"/>
						</div>

						<!-- learn more link URI -->
						<div class="editor-block">
							<label for="message" class="editor-title">Learn More URL:</label>
							<xsl:copy-of select="page/body/cookie_consent_silktide/learn_more_uri/node()"/>
						</div>

						<!-- dismiss button -->
						<div class="editor-block">
							<label for="dismiss" class="editor-title">Label of the Dimiss Button:</label>
							<xsl:copy-of select="page/body/cookie_consent_silktide/dismiss/node()"/>
						</div>
					</fieldset>

					<fieldset class="cookie-consent-settings">
						<legend>Settings</legend>

						<!-- domain -->
						<div class="editor-block">
							<label for="domain" class="editor-title">Domain used with Cookie Consent cookie:</label>
							<xsl:copy-of select="page/body/cookie_consent_silktide/domain/node()"/>
						</div>

						<!-- consent duration -->
						<div class="editor-block">
							<label for="consent_duration" class="editor-title">Keep consent for that many days:</label>
							<xsl:copy-of select="page/body/cookie_consent_silktide/consent_duration/node()"/>
						</div>
					</fieldset>

					<fieldset class="cookie-consent-theme">
						<legend>Theme</legend>

						<!-- theme widget -->
						<div class="editor-block">
							<label for="theme" class="editor-title">Select Look and Location of Widget:</label>
							<xsl:copy-of select="page/body/cookie_consent_silktide/theme/node()"/>
						</div>
					</fieldset>

				</div>
			</div>

		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
