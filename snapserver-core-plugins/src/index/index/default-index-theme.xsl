<?xml version="1.0"?>
<!--
Snap Websites Server == default index theme layout setup
Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved

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
	<xsl:param name="layout-name">default-index</xsl:param>
	<xsl:param name="layout-area">default-index-theme</xsl:param>
	<xsl:param name="layout-modified">2019-03-04 10:18:52</xsl:param>

	<xsl:template match="snap">
		<xsl:if test="count(page/body/index/*)">
			<div class="index-wrapper">
				<ul class="index default-index">
					<xsl:for-each select="page/body/index">
						<xsl:copy-of select="record/node()"/>
					</xsl:for-each>
				</ul>
				<xsl:if test="page/body/index-navigation-tags">
					<div class="navigation"><xsl:copy-of select="page/body/index-navigation-tags/node()"/></div>
				</xsl:if>
			</div>
		</xsl:if>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
