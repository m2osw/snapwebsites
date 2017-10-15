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
	<xsl:variable name="layout-area">epayment-credit-card-parser</xsl:variable>
	<xsl:variable name="layout-modified">2016-01-29 00:39:07</xsl:variable>
	<xsl:variable name="layout-editor">epayment-credit-card-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form" form_name="info" mode="save-all">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="auto-reset"><xsl:value-of select="page/body/editor/auto_reset/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<xsl:copy-of select="page/body/epayment/gateway/node()"/>

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
						<legend>Credit Card</legend>

						<div class="editor-block">
							<label for="user_name" class="editor-title">First and Last Name:</label>
							<xsl:copy-of select="page/body/epayment/user_name/node()"/>
						</div>

						<div class="editor-block">
							<label for="card_number" class="editor-title">Card Number:</label>
							<xsl:copy-of select="page/body/epayment/card_number/node()"/>
						</div>

						<div class="editor-block">
							<label for="security_code" class="editor-title">Security Code:</label>
							<xsl:copy-of select="page/body/epayment/security_code/node()"/>
						</div>

						<div class="editor-block">
							<xsl:copy-of select="page/body/epayment/expiration_date/node()"/>
						</div>
					</fieldset>

					<fieldset class="billing">
						<legend>Billing Address</legend>

						<xsl:if test="page/body/epayment/billing_business_name">
							<div class="editor-block">
								<label for="billing_business_name" class="editor-title">Business Name:</label>
								<xsl:copy-of select="page/body/epayment/billing_business_name/node()"/>
							</div>
						</xsl:if>

						<xsl:if test="page/body/epayment/billing_attention">
							<div class="editor-block">
								<label for="billing_attention" class="editor-title">Attention:</label>
								<xsl:copy-of select="page/body/epayment/billing_attention/node()"/>
							</div>
						</xsl:if>

						<div class="editor-block">
							<label for="billing_address1" class="editor-title">Address:</label>
							<xsl:copy-of select="page/body/epayment/billing_address1/node()"/>
							<xsl:copy-of select="page/body/epayment/billing_address2/node()"/>
						</div>

						<div class="editor-block">
							<label for="billing_city" class="editor-title">City:</label>
							<xsl:copy-of select="page/body/epayment/billing_city/node()"/>
						</div>

						<xsl:if test="page/body/epayment/billing_province">
							<div class="editor-block">
								<label for="billing_province" class="editor-title">Province / State:</label>
								<xsl:copy-of select="page/body/epayment/billing_province/node()"/>
							</div>
						</xsl:if>

						<div class="editor-block">
							<label for="billing_postal_code" class="editor-title">Postal Code:</label>
							<xsl:copy-of select="page/body/epayment/billing_postal_code/node()"/>
						</div>

						<xsl:if test="page/body/epayment/billing_country">
							<div class="editor-block">
								<label for="billing_country" class="editor-title">Country:</label>
								<xsl:copy-of select="page/body/epayment/billing_country/node()"/>
							</div>
						</xsl:if>
					</fieldset>

					<xsl:if test="page/body/epayment/delivery_address1">
						<fieldset class="delivery">
							<legend>Delivery Address</legend>

							<xsl:if test="page/body/epayment/delivery_business_name">
								<div class="editor-block">
									<label for="delivery_business_name" class="editor-title">Business Name:</label>
									<xsl:copy-of select="page/body/epayment/delivery_business_name/node()"/>
								</div>
							</xsl:if>

							<xsl:if test="page/body/epayment/delivery_attention">
								<div class="editor-block">
									<label for="delivery_attention" class="editor-title">Attention:</label>
									<xsl:copy-of select="page/body/epayment/delivery_attention/node()"/>
								</div>
							</xsl:if>

							<div class="editor-block">
								<label for="delivery_address1" class="editor-title">Address:</label>
								<xsl:copy-of select="page/body/epayment/delivery_address1/node()"/>
								<xsl:copy-of select="page/body/epayment/delivery_address2/node()"/>
							</div>

							<div class="editor-block">
								<label for="delivery_city" class="editor-title">City:</label>
								<xsl:copy-of select="page/body/epayment/delivery_city/node()"/>
							</div>

							<xsl:if test="page/body/epayment/delivery_province">
								<div class="editor-block">
									<label for="delivery_province" class="editor-title">Province / State:</label>
									<xsl:copy-of select="page/body/epayment/delivery_province/node()"/>
								</div>
							</xsl:if>

							<div class="editor-block">
								<label for="delivery_postal_code" class="editor-title">Postal Code:</label>
								<xsl:copy-of select="page/body/epayment/delivery_postal_code/node()"/>
							</div>

							<xsl:if test="page/body/epayment/delivery_country">
								<div class="editor-block">
									<label for="delivery_country" class="editor-title">Country:</label>
									<xsl:copy-of select="page/body/epayment/delivery_country/node()"/>
								</div>
							</xsl:if>
						</fieldset>
					</xsl:if>

					<xsl:if test="page/body/epayment/phone">
						<fieldset class="other">
							<legend>Other Information</legend>

							<xsl:if test="page/body/epayment/phone">
								<div class="editor-block">
									<label for="phone" class="editor-title">Phone:</label>
									<xsl:copy-of select="page/body/epayment/phone/node()"/>
								</div>
							</xsl:if>
						</fieldset>
					</xsl:if>

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
