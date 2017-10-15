<?xml version="1.0"?>
<!--
Snap Websites Server == e-Payment Stripe Express Checkout settings parser for our test form
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
	<xsl:variable name="layout-area">epayment-stripe-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2016-01-27 00:16:18</xsl:variable>
	<xsl:variable name="layout-editor">epayment-stripe-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output><!-- lang="{$lang}"-->
			<div class="editor-form" form_name="epayment_stripe">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<!-- xsl:if test="$action != 'edit' and $can_edit = 'yes'">
					<a class="settings-edit-button" href="?a=edit">Edit</a>
				</xsl:if>
				<xsl:if test="$action = 'edit'">
					<a class="settings-save-button" href="#">Save Changes</a>
					<a class="settings-cancel-button right-aligned" href="{/snap/head/metadata/desc[@type='page_uri']/data}">Cancel</a>
				</xsl:if-->
				<!--h2>Stripe Settings</h2-->
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset>
						<legend>Stripe Access Codes</legend>

						<div class="editor-block">
							<label for="secret" class="settings-title">Live Secret Key (sk_live_...): <span class="required">*</span></label>
							<xsl:copy-of select="page/body/epayment_stripe/secret/node()"/>
						</div>

						<div class="editor-block">
							<label for="maximum_repeat_failures" class="settings-title">Maximum Repeat Failures</label>
							<xsl:copy-of select="page/body/epayment_stripe/maximum-repeat-failures/node()"/>
						</div>
					</fieldset>

					<fieldset>
						<legend>Stripe Debug</legend>

						<!-- stripe debug widget -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/epayment_stripe/debug/node()"/>
						</div>

						<div class="editor-block">
							<label for="secret" class="settings-title">Test Secret Key (sk_test_...):</label>
							<xsl:copy-of select="page/body/epayment_stripe/test_secret/node()"/>
							<p>
								You may use the stripe public key:
								<strong>sk_test_BQokikJOvBiI2HlWgH4olfQ2</strong>
								which anyone has access to and may end up using... so when
								using that key, make sure not to enter data which you do
								not want to become public.
							</p>
							<p>
								Pretty much all the data you enter will be ignored when
								in debug mode (although presence is important and enforced
								by Snap! anyway.) However, the only accepted card numbers
								while testing is limited. For example, you can use
								4242-4242-4242-4242 and others as defined on the
								<a href="https://stripe.com/docs/testing"
								target="_blank">Testing</a> page of stripe.
							</p>
						</div>

					</fieldset>

				</div>
			</div>

		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
