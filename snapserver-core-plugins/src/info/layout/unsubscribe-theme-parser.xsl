<?xml version="1.0"?>
<!--
Snap Websites Server == default theme layout setup
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
  <xsl:include href="html-header"/>

  <!-- some special variables to define the theme -->
	<xsl:param name="layout-name">sendmail</xsl:param>
	<xsl:param name="layout-area">sendmail-theme-parser</xsl:param>
	<xsl:param name="layout-modified">2015-11-03 16:38:57</xsl:param>

	<xsl:template match="snap">
		<html>
      <xsl:call-template name="snap:html-attributes"/>
			<head>
        <xsl:call-template name="snap:html-header"/>
				<style>
					body
					{
						font-family: sans-serif;
						background: white;
					}

					body, div
					{
						padding: 0;
						margin: 0;
					}

					h1
					{
						font-size: 150%;
					}
					h2
					{
						font-size: 130%;
					}
					h3
					{
						font-size: 115%;
					}

					.page
					{
						padding: 10px;
					}

					.header
					{
						height: 65px;
						border-bottom: 1px solid #666666;
						margin-bottom: 20px;
					}

					.header h1
					{
						text-align: center;
						font-size: 250%;
						padding-top: 10px;
					}

					.left
					{
						float: left;
						padding-right: 10px;
						width: 239px;
						min-height: 350px;
						border-right: 1px solid #666666;
					}

					.content
					{
						float: left;
						width: 730px;
						padding: 10px;
					}

					.clear-both
					{
						clear: both;
					}

					.inner-page
					{
					}

					.content .body
					{
					}

					.footer
					{
						margin-top: 20px;
						padding: 10px;
						border-top: 1px solid #666666;
						text-align: center;
						color: #888888;
						font-size: 80%;
					}

					.error input
					{
						color: #ff0000;
					}

					.left .box input.line-edit-input,
					.left .box input.password-input
					{
						display: block;
						width: 150px;
					}

					.input-with-background-value
					{
						color: #888888;
					}
				</style>
			</head>
			<xsl:apply-templates select="page/body"/>
		</html>
	</xsl:template>
	<xsl:template match="page/body">
		<body>
			<div class="page">
				<div class="header"><h1 style="font-size: 150%;"><xsl:choose>
						<xsl:when test="/snap/page/body/titles/long-title">
							<xsl:copy-of select="/snap/page/body/titles/long-title/node()"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:copy-of select="/snap/page/body/titles/title/node()"/>
						</xsl:otherwise>
					</xsl:choose></h1></div>
				<div><xsl:copy-of select="output/node()"/></div>
				<div style="clear: both;"></div>
			</div>
		</body>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
