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
	<xsl:param name="layout-name">default</xsl:param>
	<xsl:param name="layout-area">theme</xsl:param>
	<xsl:param name="layout-modified">2012-10-30 10:41:35</xsl:param>

	<xsl:template match="snap">
		<html>
      <xsl:call-template name="snap:html-attributes"/>
			<head>
        <xsl:call-template name="snap:html-header"/>
				<style>
					body, div
					{
						margin: 0;
						padding: 0;
					}
					div.page
					{
					}
					div.column
					{
						float: left;
					}
					div.column.content
					{
						margin-left: -360px;
						width: 100%;
					}
					div.squeezed
					{
						margin-left: 360px;
					}
					div.column.left
					{
						position: relative;
						width: 340px;
						padding-right: 20px;
						z-index: 2;
					}
					div.user-messages
					{
						border: 1px solid #550000;
						background-color: #fff0f0;
						padding: 0 10px;
						margin-bottom: 20px;
					}
					div.message h3
					{
						color: #770033;
					}
					div.user-messages .close-button
					{
						display: none;
					}
					.editor-content[contenteditable="true"] .filter-token
					{
						background-color: #e0e0e0;
					}
				</style>
			</head>
			<xsl:apply-templates select="page/body"/>
		</html>
	</xsl:template>
	<xsl:template match="page/body">
		<body>
			<div class="page">
				<div class="column left">
					<xsl:for-each select="/snap/page/boxes/left">
						<div class="box">
							<!-- copy nodes under left -->
							<h2 class="box-title"><xsl:choose>
								<xsl:when test="descendant::node()/titles/short-title">
									<xsl:value-of select="*/titles/short-title"/>
								</xsl:when>
								<xsl:otherwise>
									<xsl:value-of select="*/titles/title"/>
								</xsl:otherwise>
								</xsl:choose></h2>
							<div class="box-content">
								<xsl:copy-of select="descendant::node()/content/node()"/>
							</div>
						</div>
					</xsl:for-each>
				</div>
				<div class="column content">
					<div class="squeezed">
						<!-- TODO: this is totally wrong because we may be editing the
						           long-title or [normal] title when those are two
											 distinct fields -->
						<div><div><xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor</xsl:if></xsl:attribute>
						     <xsl:attribute name="field_name">title</xsl:attribute><h2 class="editor-content"><xsl:choose>
								<xsl:when test="/snap/page/body/titles/long-title">
									<xsl:copy-of select="/snap/page/body/titles/long-title/node()"/>
								</xsl:when>
								<xsl:otherwise>
									<xsl:copy-of select="/snap/page/body/titles/title/node()"/>
								</xsl:otherwise>
							</xsl:choose></h2></div></div>
						<div class="breadcrumb"><xsl:copy-of select="/snap/page/body/breadcrumb/node()"/></div>
						<xsl:copy-of select="output/node()"/>
					</div>
				</div>
				<div style="clear: both;"></div>
			</div>
		</body>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
