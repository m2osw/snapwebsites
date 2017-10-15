<?xml version="1.0"?>
<!--
Snap Websites Server == generate a list.xml of PADFile files
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

	<!-- some special variables to define the theme -->
	<xsl:variable name="layout-name">padlist</xsl:variable>
	<xsl:variable name="layout-area">padlist-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-11-29 16:02:24</xsl:variable>

	<xsl:template match="file">
		<Application_XML_File_URL><xsl:value-of select="."/>/padfile.xml</Application_XML_File_URL>
	</xsl:template>

	<xsl:template match="snap">
		<XML_DIZ_LIST>
			<MASTER_PAD_VERSION_INFO>
				<MASTER_PAD_VERSION>3.11</MASTER_PAD_VERSION>
				<MASTER_PAD_EDITOR>Snap! Websites <xsl:value-of select="@version"/></MASTER_PAD_EDITOR>
				<MASTER_PAD_INFO>http://www.asp-shareware.org/pad/spec/spec.php</MASTER_PAD_INFO>
			</MASTER_PAD_VERSION_INFO>
			<Application_XML_Files>
				<xsl:apply-templates select="file"/>
			</Application_XML_Files>
		</XML_DIZ_LIST>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
