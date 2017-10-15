<?xml version="1.0"?>
<!--
Snap Websites Server == default body layout setup
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
	<xsl:include href="user-messages"/>

  <!-- some special variables to define the theme -->
	<xsl:param name="layout-name">default</xsl:param>
	<xsl:param name="layout-area">body</xsl:param>
	<xsl:param name="layout-modified">2012-10-30 10:41:35</xsl:param>

	<xsl:template match="snap">
		<output lang="{$lang}">
			<div id="content">
				<xsl:call-template name="snap:user-messages"/>
				<div field_name="body"><xsl:attribute name="class"><xsl:if test="$action = 'edit'">snap-editor</xsl:if></xsl:attribute>
					 <div class="editor-content"><xsl:copy-of select="page/body/content/node()"/></div></div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
