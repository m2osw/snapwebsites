<?xml version="1.0"?>
<!--
Snap Websites Server == QR Code settings parser for our test form
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
	<!-- include system data -->
	<xsl:include href="functions"/>

	<!-- some special variables to define the theme -->
	<xsl:variable name="layout-area">qrcode-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-01-23 02:51:10</xsl:variable>
	<xsl:variable name="layout-editor">qrcode-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output lang="{$lang}">
			<div class="editor-form" form_name="qrcode_settings">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<p>To generate the QR Code of a page, use an image tag as follow:</p>
				<pre>    &lt;img src="/images/qrcode/path/to/page"/&gt;</pre>
				<p>Simply replace <b>path/to/page</b> with the real path to your
				   page and the module will automatically return the QR Code for
					 that page. You can only generate codes for your own website.
					 To generate a QR Code for your home page use <b>index.html</b>
					 as the <b>path/to/page</b> parameter.</p>
				<p>You may force the scale by using the query string parameter
				   named <b>scale</b>. The default is <code>...?scale=3</code>.
					 The scale must be one of: 1, 2, 3, 4, or 5.</p>
				<p>You may force the width of the edge by using the query string
				   parameter named <b>edge</b>. The default is
					 <code>...?edge=15</code>.
					 The edge must be defined between 0 and 50 inclusive.</p>
				<p>You may first generate the image, then check the size and specify
				   the width and height parameters of the image in the tag.
					 Those attributes are not mandatory, but may help the browser to
					 properly size the page elements even before getting the QR Code.</p>

				<fieldset>
					<legend>Basics</legend>

					<div class="editor-block">
						<xsl:copy-of select="page/body/qrcode/private_enable/node()"/>
					</div>
					<div class="editor-block">
						<xsl:copy-of select="page/body/qrcode/shorturl_enable/node()"/>
					</div>
					<div class="editor-block">
						<xsl:copy-of select="page/body/qrcode/track_usage_enable/node()"/>
					</div>

					<div class="settings-title">
						<label for="qrcode_default_scale" class="company_plan">Scale</label>
						<xsl:copy-of select="page/body/qrcode/default_scale/node()"/>
					</div>

					<div class="settings-title">
						<label for="qrcode_default_edge" class="company_plan">Edge (space around the QR code)</label>
						<xsl:copy-of select="page/body/qrcode/default_edge/node()"/>
					</div>
				</fieldset>

			</div>

		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
