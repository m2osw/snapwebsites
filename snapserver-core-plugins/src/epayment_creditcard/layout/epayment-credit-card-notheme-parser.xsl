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
	<xsl:variable name="layout-area">epayment-credit-card-notheme-parser</xsl:variable>
	<xsl:variable name="layout-modified">2016-04-05 13:20:07</xsl:variable>
	<xsl:variable name="layout-editor">epayment-credit-card-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form" form_name="credit_card_form" mode="save-all">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="auto-reset"><xsl:value-of select="page/body/editor/auto_reset/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<xsl:copy-of select="page/body/epayment/gateway/node()"/>
				<xsl:copy-of select="page/body/epayment/from/node()"/>

				<ul class="epayment-credit-card-tab-list">
					<li class="active" for="card-info">Card Info</li>
					<xsl:choose>
						<xsl:when test="page/body/epayment/delivery_address1">
							<li for="billing">Billing Address</li>
							<li for="delivery">Delivery Address</li>
						</xsl:when>
						<xsl:otherwise>
							<li for="billing">Address</li>
						</xsl:otherwise>
					</xsl:choose>
					<xsl:if test="page/body/epayment/phone">
						<li for="other">Other</li>
					</xsl:if>
					<div class="clear-both"/>
				</ul>

				<div class="tab-selector">

					<div data-name="card-info" class="epayment-credit-card-tab">
						<div class="editor-block epayment-credit-card-user-name">
							<div class="epayment-credit-card-user-name-label">
								<label for="user_name" class="editor-title">Name on card: <span class="required">*</span></label>
							</div>
							<div class="epayment-credit-card-user-name-widget">
								<xsl:copy-of select="page/body/epayment/user_name/node()"/>
							</div>
						</div>

						<div class="epayment-credit-card-info">
							<div class="editor-block epayment-card-number">
								<label for="card_number" class="editor-title">Card Number: <span class="required">*</span></label>
								<xsl:copy-of select="page/body/epayment/card_number/node()"/>
							</div>
							<div class="editor-block epayment-card-security-code">
								<label for="security_code" class="editor-title">Security Code: <span class="required">*</span></label>
								<xsl:copy-of select="page/body/epayment/security_code/node()"/>
							</div>
						</div>

						<div class="editor-block">
							<xsl:copy-of select="page/body/epayment/expiration_date/node()"/>
						</div>
					</div>

					<div data-name="billing" class="epayment-credit-card-tab">
						<xsl:if test="page/body/epayment/billing_business_name or page/body/epayment/billing_attention">
							<div class="epayment-credit-card-billing-name">
								<xsl:if test="page/body/epayment/billing_business_name">
									<div class="editor-block epayment-credit-card-business-name">
										<label for="billing_business_name" class="editor-title">Business Name:</label>
										<xsl:copy-of select="page/body/epayment/billing_business_name/node()"/>
									</div>
								</xsl:if>
								<xsl:if test="page/body/epayment/billing_attention">
									<div class="editor-block epayment-credit-card-attention">
										<label for="billing_attention" class="editor-title">Attention:</label>
										<xsl:copy-of select="page/body/epayment/billing_attention/node()"/>
									</div>
								</xsl:if>
							</div>
						</xsl:if>

						<div class="epayment-credit-card-billing-address">
							<div class="editor-block epayment-credit-card-address-label">
								<label for="billing_address1" class="editor-title">Address:&#160;<span class="required">*</span></label>
							</div>
							<div class="editor-block epayment-credit-card-address-widgets">
								<xsl:copy-of select="page/body/epayment/billing_address1/node()"/>
								<xsl:copy-of select="page/body/epayment/billing_address2/node()"/>
							</div>
						</div>

						<div class="epayment-credit-card-billing-address">
							<div class="editor-block epayment-billing-city">
								<label for="billing_city" class="editor-title">City: <span class="required">*</span></label>
								<xsl:copy-of select="page/body/epayment/billing_city/node()"/>
							</div>

							<xsl:if test="page/body/epayment/billing_province">
								<div class="editor-block epayment-billing-province">
									<label for="billing_province" class="editor-title">Province / State:</label>
									<xsl:copy-of select="page/body/epayment/billing_province/node()"/>
								</div>
							</xsl:if>

							<div class="editor-block epayment-billing-postal-code">
								<label for="billing_postal_code" class="editor-title">Postal Code: <span class="required">*</span></label>
								<xsl:copy-of select="page/body/epayment/billing_postal_code/node()"/>
							</div>
						</div>

						<xsl:if test="page/body/epayment/billing_country">
							<div class="epayment-credit-card-billing-country">
								<div class="editor-block epayment-credit-card-country-label">
									<label for="billing_country" class="editor-title">Country:&#160;<span class="required">*</span></label>
								</div>
								<div class="editor-block epayment-credit-card-country-widget">
									<xsl:copy-of select="page/body/epayment/billing_country/node()"/>
								</div>
							</div>
						</xsl:if>
					</div>

					<xsl:if test="page/body/epayment/delivery_address1">
						<div data-name="delivery" class="epayment-credit-card-tab">
							<xsl:if test="page/body/epayment/delivery_business_name or page/body/epayment/delivery_attention">
								<div class="epayment-credit-card-delivery-name">
									<div class="editor-block epayment-credit-card-business-name">
										<label for="delivery_business_name" class="editor-title">Business Name:</label>
										<xsl:copy-of select="page/body/epayment/delivery_business_name/node()"/>
									</div>
									<xsl:if test="page/body/epayment/delivery_attention">
										<div class="editor-block epayment-credit-card-attention">
											<label for="delivery_attention" class="editor-title">Attention:</label>
											<xsl:copy-of select="page/body/epayment/delivery_attention/node()"/>
										</div>
									</xsl:if>
								</div>
							</xsl:if>

							<div class="epayment-credit-card-delivery-address">
								<div class="editor-block epayment-credit-card-address-label">
									<label for="delivery_address1" class="editor-title">Address:</label>
								</div>
								<div class="editor-block epayment-credit-card-address-widgets">
									<xsl:copy-of select="page/body/epayment/delivery_address1/node()"/>
									<xsl:copy-of select="page/body/epayment/delivery_address2/node()"/>
								</div>
							</div>

							<div class="epayment-credit-card-delivery-address">
								<div class="editor-block epayment-delivery-city">
									<label for="delivery_city" class="editor-title">City:</label>
									<xsl:copy-of select="page/body/epayment/delivery_city/node()"/>
								</div>

								<xsl:if test="page/body/epayment/delivery_province">
									<div class="editor-block epayment-delivery-province">
										<label for="delivery_province" class="editor-title">Province / State:</label>
										<xsl:copy-of select="page/body/epayment/delivery_province/node()"/>
									</div>
								</xsl:if>

								<div class="editor-block epayment-delivery-postal-code">
									<label for="delivery_postal_code" class="editor-title">Postal Code:</label>
									<xsl:copy-of select="page/body/epayment/delivery_postal_code/node()"/>
								</div>
							</div>

							<xsl:if test="page/body/epayment/delivery_country">
								<div class="epayment-credit-card-delivery-country">
									<div class="editor-block epayment-credit-card-country-label">
										<label for="delivery_country" class="editor-title">Country:</label>
									</div>
									<div class="editor-block epayment-credit-card-country-widget">
										<xsl:copy-of select="page/body/epayment/delivery_country/node()"/>
									</div>
								</div>
							</xsl:if>
						</div>
					</xsl:if>

					<xsl:if test="page/body/epayment/phone">
						<div data-name="other" class="epayment-credit-card-tab">
							<xsl:if test="page/body/epayment/phone">
								<div class="editor-block">
									<label for="country" class="editor-title">Phone:</label>
									<xsl:copy-of select="page/body/epayment/phone/node()"/>
								</div>
							</xsl:if>
						</div>
					</xsl:if>

				</div> <!-- tab-selector -->

				<div class='epayment-buttons'>
					<a class="cancel epayment-cancel" href="#">Cancel</a>
					<span class="send-button-wrapper" style="display: none;"><a class="send epayment-send" style="display: none;" href="#">Send</a></span>
					<a class="next epayment-next" style="display: none;" href="#">Next »</a>
					<a class="back epayment-back" style="display: none;" href="#">« Back</a>
					<div class='clear-both'></div>
				</div>

			</div>
			<css name="epayment-credit-card"/>
			<javascript name="epayment-credit-card-form"/>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
