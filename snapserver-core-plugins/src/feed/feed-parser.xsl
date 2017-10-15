<?xml version="1.0"?>
<!--
Snap Websites Server == feed page data to XML
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
	<xsl:variable name="layout-name">feed</xsl:variable>
	<xsl:variable name="layout-area">feed-parser</xsl:variable>
	<xsl:variable name="layout-modified">2014-11-29 02:35:36</xsl:variable>

	<xsl:template match="snap">
		<output lang="{$lang}">
			<!-- the path is probably wrong, we may want to use $base_uri instead? -->
			<url><xsl:copy-of select="$page_uri"/></url>
			<xsl:copy-of select="page/body/titles"/>
			<xsl:copy-of select="page/body/author"/>
			<xsl:copy-of select="page/body/created-precise"/>
			<xsl:copy-of select="page/body/modified-precise"/>
			<created-long-date><xsl:copy-of select="page/body/create-precised/node()"/></created-long-date>
			<xsl:if test="page/body/owner != ''">
				<copyright>Copyright &#xA9; <xsl:copy-of select="$year_range"/> by <xsl:copy-of select="page/body/owner"/></copyright>
			</xsl:if>
			<description>
				<xsl:choose>
					<xsl:when test="page/body/feed">
						<!-- special content for the feed, use that instead of the
								 default body content -->
						<xsl:copy-of select="page/body/feed/node()"/>
					</xsl:when>
					<xsl:otherwise>
						<div id="content">
							<xsl:copy-of select="page/body/content/node()"/>
						</div>
					</xsl:otherwise>
				</xsl:choose>
			</description>
			<!--
				TODO:
					category == we need a way to retrieve a tag clearly marked as a category
					            <category domain="http://example.com/types/taxonomy/category/foo">Foo</category>
											the category itself can be a path:
					            <category domain="http://example.com/types/taxonomy/category/foo/blah">Foo/Blah</category>
											we can add multiple <category ...> tags in each item/channel
					comments == whether user can add comments (the URL will just be <url>#comments)
					enclosure == to add media (i.e. mp3) to feeds
					             <enclosure url="http://example.com/video/feed.mov" length="1000000" type="video/mpeg4"/>
			-->
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
