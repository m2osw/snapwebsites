<?xml version="1.0"?>
<!--
Snap Websites Server == feed page data to RSS XML
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
															xmlns:snap="span:snap">
  <!-- get the year the page was created -->
  <xsl:variable name="year" select="year-from-date(current-date())"/>
  <xsl:variable name="created_year" select="year-from-date(/snap/page/body/created)"/>
  <xsl:variable name="year_range">
		<xsl:value-of select="$created_year"/><xsl:if test="$created_year != xs:integer($year)">-<xsl:value-of select="$year"/></xsl:if>
  </xsl:variable>

	<xsl:template match="output">
		<item>
			<title><xsl:copy-of select="titles/title/node()"/></title>
			<link><xsl:copy-of select="url/node()"/></link>
			<description feed-cdata="yes"><xsl:copy-of select="description/node()"/></description>
			<xsl:copy-of select="author"/>
			<!--xsl:for-each select="category">
				<category>
					...
				</category>
			</xsl:for-each-->
			<!-- TODO: check whether comments are allowed on that page -->
			<comments><xsl:copy-of select="url/node()"/>#comments</comments>
			<!--enclosure>...</enclosure-->
			<guid><xsl:copy-of select="url/node()"/></guid>
			<pubDate><xsl:copy-of select="created-long-date/node()"/></pubDate>
			<source url="{/snap/head/metadata/desc[@type='website_uri']/data/node()}"><xsl:copy-of select="/snap/head/metadata/desc[@type='name']/data/node()"/></source>
		</item>
	</xsl:template>

	<xsl:template match="snap">
		<rss version="2.0" ns="xmlns:atom=http://www.w3.org/2005/Atom" xmlns:atom="http://www.w3.org/2005/Atom">
			<snap-info extension="rss" mimetype="application/rss+xml">
				<title><xsl:copy-of select="head/metadata/desc[@type='name']/data/node()"/></title>
			</snap-info>
			<channel>
				<title><xsl:copy-of select="head/metadata/desc[@type='name']/data/node()"/></title>
				<link><xsl:copy-of select="head/metadata/desc[@type='website_uri']/data/node()"/></link>
				<atom:link href="{head/metadata/desc[@type='feed::uri']/data/node()}/{head/metadata/desc[@type='feed::name']/data/node()}.rss" rel="self" type="application/rss+xml"/>
				<!-- the main description cannot include HTML -->
				<description><xsl:value-of select="head/metadata/desc[@type='description']/data/node()"/></description>
				<language><xsl:value-of select="page/body/output[1]/@lang"/></language>
				<xsl:if test="page/body/owner != ''">
					<copyright>Copyright &#xA9; <xsl:copy-of select="$year_range"/> by <xsl:copy-of select="page/body/owner"/></copyright>
				</xsl:if>
				<!--managingEditor><xsl:copy-of select=""/><managingEditor-->
				<!--webMaster><xsl:copy-of select=""/><webMaster-->
				<!-- TODO: change pubDate with the last publication date of the data instead -->
				<pubDate><xsl:value-of select="head/metadata/desc[@type='feed::now-long-date']/data/node()"/></pubDate>
				<lastBuildDate><xsl:value-of select="head/metadata/desc[@type='feed::now-long-date']/data/node()"/></lastBuildDate>
				<!--category>...</category-->
				<generator>Snap! Websites <xsl:value-of select="head/metadata/desc[@type='version']/data/node()"/></generator>
				<docs>http://www.rssboard.org/rss-specification</docs>
				<!--cloud>...</cloud-->
				<ttl><xsl:choose>
						<xsl:when test="head/metadata/desc[@type='ttl']/data">
							<!-- user defined -->
							<xsl:value-of select="head/metadata/desc[@type='ttl']/data/node()"/>
						</xsl:when>
						<!-- default is 1 week -->
						<xsl:otherwise>10080</xsl:otherwise>
					</xsl:choose></ttl>
				<xsl:if test="head/metadata/desc[@type='feed::default_logo']/data">
					<image>
						<title><xsl:copy-of select="head/metadata/desc[@type='name']/data/node()"/></title>
						<link><xsl:copy-of select="head/metadata/desc[@type='website_uri']/data/node()"/></link>
						<!-- the description becomes the img title attribute -->
						<description feed-cdata="yes"><xsl:value-of select="head/metadata/desc[@type='description']/data/node()"/></description>
						<url><xsl:value-of select="head/metadata/desc[@type='feed::default_logo']/data/img/@src"/></url>
						<width><xsl:value-of select="head/metadata/desc[@type='feed::default_logo']/data/img/@width"/></width>
						<height><xsl:value-of select="head/metadata/desc[@type='feed::default_logo']/data/img/@height"/></height>
					</image>
				</xsl:if>
				<!--rating>PICS rating</rating-->

				<xsl:apply-templates select="page/body/output"/>
			</channel>
		</rss>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
