<?xml version="1.0"?>
<!--
Snap Websites Server == filter token help parser
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
	<xsl:variable name="calendar">token-help</xsl:variable>
	<xsl:variable name="calendar-modified">2016-03-19 23:04:52</xsl:variable>

	<xsl:template match="token">
		<li>
			<span class="token-name"><xsl:value-of select="@name"/></span>
			&#x2014;
			<span class="token-description"><xsl:copy-of select="."/></span>
		</li>
	</xsl:template>

	<xsl:template match="snap">
		<output> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div id="token-help">

				<h3>Token Help</h3>

				<ul class="token-list">
					<xsl:apply-templates select="token-help/*"/>
				</ul>

			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
