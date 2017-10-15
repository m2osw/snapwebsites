<?xml version="1.0"?>
<!--
Snap Websites Server == default list theme layout setup
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
	<xsl:param name="layout-name">default-list</xsl:param>
	<xsl:param name="layout-area">default-list-theme</xsl:param>
	<xsl:param name="layout-modified">2015-10-14 21:33:24</xsl:param>

	<xsl:template match="snap">
		<xsl:if test="count(page/body/list/*)">
			<div class="list-wrapper">
				<ul class="list default-list">
					<xsl:for-each select="page/body/list">
						<xsl:copy-of select="item/node()"/>
					</xsl:for-each>
				</ul>
				<xsl:if test="page/body/list-navigation-tags">
					<div class="navigation"><xsl:copy-of select="page/body/list-navigation-tags/node()"/></div>
				</xsl:if>
			</div>
		</xsl:if>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
