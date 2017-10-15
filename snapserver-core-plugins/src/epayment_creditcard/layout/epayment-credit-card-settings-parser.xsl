<?xml version="1.0"?>
<!--
Snap Websites Server == epayment credit catd settings parser
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
	<xsl:variable name="layout-area">epayment-credit-card-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2016-04-05 14:10:09</xsl:variable>
	<xsl:variable name="layout-editor">epayment-credit-card-settings-page</xsl:variable>

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
				<h3>Billing Information</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset class="site-name">
						<legend>Credit Card Form</legend>

						<div class="editor-block">
							<xsl:copy-of select="page/body/epayment/credit_card/show_one_name/node()"/>
							<xsl:copy-of select="page/body/epayment/credit_card/show_business_name/node()"/>
							<xsl:copy-of select="page/body/epayment/credit_card/show_delivery/node()"/>
							<xsl:copy-of select="page/body/epayment/credit_card/show_address2/node()"/>
							<xsl:copy-of select="page/body/epayment/credit_card/show_province/node()"/>
						</div>

						<div class="editor-block">
							<xsl:copy-of select="page/body/epayment/credit_card/show_country/node()"/>
							<label for="security_code" class="editor-title">Default Country:</label>
							<xsl:copy-of select="page/body/epayment/credit_card/default_country/node()"/>
						</div>

						<div class="editor-block">
							<label>Phone Field</label>
							<xsl:copy-of select="page/body/epayment/credit_card/show_phone/node()"/>
						</div>

					</fieldset>

					<fieldset class="site-name">
						<legend>Credit Card Gateway</legend>

						<div class="editor-block">
							<xsl:copy-of select="page/body/epayment/credit_card/allow_credit_card_tokens/node()"/>
						</div>

						<div class="editor-block">
							<label>Default Gateway</label>
							<xsl:copy-of select="page/body/epayment/credit_card/gateway/node()"/>
							<p>
								<strong>Note:</strong> If you do not select a default
								Gateway, then all the form asking for credit card
								details must be called with ?gateway=&lt;plugin-name&gt;
								otherwise an error will be generated and the end user
								will get a 503 error.
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
