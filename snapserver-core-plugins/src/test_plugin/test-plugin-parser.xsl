<?xml version="1.0"?>
<!--
Snap Websites Server == group and test to HTML parser
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

	<xsl:template match="test">
		<div>
			<xsl:attribute name="class">
				test
				<xsl:value-of select="@ran"/>
				<xsl:text> </xsl:text>
				<xsl:if test="@ran = 'ran' and @success = 1">succeeded</xsl:if>
				<xsl:if test="@ran = 'ran' and @success = 0">failed</xsl:if>
			</xsl:attribute>
			<xsl:value-of select="@count"/>.
			<a class="run name" href="{@name}" title="Click to run this specific test."><xsl:value-of select="@name"/></a>
			<span class="status">
				<xsl:if test="@ran = 'ran'">
					| Last run on <span title="It run until {@end_date}"><xsl:value-of select="@start_date"/></span>
					for <xsl:value-of select="@duration"/> seconds
					and
					<xsl:if test="@success = 1">succeeded.</xsl:if>
					<xsl:if test="@success != 1">failed.</xsl:if>
				</xsl:if>
			</span>
		</div>
	</xsl:template>

	<xsl:template match="group[@name!='all']">
		<div class="group">
			<div class="group-title">
				Group: <a class="run" href="{@name}" title="Click to run all the tests in this group and its sub-groups."><xsl:value-of select="@name"/></a>
			</div>
			<div class="group-tests">
				<xsl:apply-templates select="test|group"/>
			</div>
		</div><!-- group -->
	</xsl:template>

	<xsl:template match="group[@name='all']">
		<div class="test-plugin">
			<div class="group">
				<div class="group-title">Group: <a class="run" href="#all" title="Click to run all the tests.">all</a></div>
				<xsl:apply-templates select="group"/>
			</div><!-- group(all) -->
		</div><!-- test-plugin -->
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
