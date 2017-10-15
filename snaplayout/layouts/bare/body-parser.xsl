<?xml version="1.0"?>
<!--
Snap Websites Server == bare body layout setup
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
<!-- to install: snaplayout bare-body-parser.xsl -->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
															xmlns:xs="http://www.w3.org/2001/XMLSchema"
															xmlns:fn="http://www.w3.org/2005/xpath-functions"
															xmlns:snap="snap:snap">
	<!-- include system data -->
	<xsl:include href="functions"/>
	<xsl:include href="user-messages"/>

  <!-- some special variables to define the theme -->
	<xsl:variable name="layout-name">bare</xsl:variable>
	<xsl:variable name="layout-area">body-parser</xsl:variable>
	<xsl:variable name="layout-modified">2014-12-03 09:44:54</xsl:variable>

	<xsl:template match="snap">
		<output lang="{$lang}">
			<div id="content">
				<!-- add the messages at the top -->
				<xsl:call-template name="snap:user-messages"/>
				<div field_type="text-edit" field_name="body"><xsl:attribute name="class"><xsl:if test="$action = 'administer'">snap-editor</xsl:if></xsl:attribute>
					 <div class="editor-content" name="body"><xsl:copy-of select="page/body/content/node()"/></div></div>
			</div>
			<div id="footer">Bare Footer</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
