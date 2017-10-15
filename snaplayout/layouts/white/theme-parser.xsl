<?xml version="1.0"?>
<!--
Snap Websites Server == white layout theme setup
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

	<xsl:variable name="layout-name">white</xsl:variable>
	<xsl:variable name="layout-area">theme-parser</xsl:variable>
	<xsl:variable name="layout-modified">2012-10-30 10:41:36</xsl:variable>

  <xsl:include href="user-messages"/>

	<xsl:param name="year" select="year-from-date(current-date())"/>
	<xsl:param name="use_dcterms">yes</xsl:param>
	<!-- get the website URI (i.e. URI without any folder other than the website base folder) -->
	<xsl:variable name="website_uri" as="xs:string" select="snap/head/metadata/desc[@type='website_uri']/data"/>
		<!--xsl:for-each select="snap">
			<xsl:value-of select="head/metadata/desc[@type='website_uri']/data"/>
		</xsl:for-each-->
	<!--/xsl:variable-->
	<!-- compute the protocol from the main URI -->
	<xsl:variable name="protocol" as="xs:string">
		<xsl:value-of select="substring-before($website_uri, '://')"/>
	</xsl:variable>
	<!-- compute the domain from the main URI -->
	<xsl:variable name="domain" as="xs:string">
		<xsl:choose>
			<xsl:when test="contains($website_uri, '://www.')">
				<xsl:value-of select="substring-before(substring-after($website_uri, '://www.'), '/')"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="substring-before(substring-after($website_uri, '://'), '/')"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>
	<!-- get the base URI (i.e. parent URI to this page) -->
	<xsl:variable name="base_uri" as="xs:string" select="snap/head/metadata/desc[@type='base_uri']/data">
		<!--xsl:for-each select="snap">
			<xsl:value-of select="head/metadata/desc[@type='base_uri']/data"/>
		</xsl:for-each-->
	</xsl:variable>
	<!-- compute the path from the main URI to this page -->
	<xsl:variable name="path" as="xs:string">
		<xsl:value-of select="substring-after($base_uri, $website_uri)"/>
	</xsl:variable>
	<!-- compute the full path from the main URI to this page -->
	<xsl:variable name="full_path" as="xs:string">
		<xsl:value-of select="concat(substring-after(substring-after($website_uri, '://'), '/'), $path)"/>
	</xsl:variable>
	<!-- get the page URI (i.e. the full path to this page) -->
	<xsl:variable name="page_uri" as="xs:string" select="head/metadata/desc[@type='page_uri']/data">
		<!--xsl:for-each select="snap">
			<xsl:value-of select="head/metadata/desc[@type='page_uri']/data"/>
		</xsl:for-each-->
	</xsl:variable>
	<!-- compute the full path from the main URI to this page -->
	<xsl:variable name="page" as="xs:string">
		<xsl:value-of select="substring-after($page_uri, $base_uri)"/>
	</xsl:variable>
	<!-- get the year the page was created -->
	<xsl:variable name="created_year" select="year-from-date(snap/page/body/created)"/>
	<xsl:variable name="year_range">
		<xsl:if test="$created_year != xs:integer($year)"><xsl:value-of select="$year"/>-</xsl:if><xsl:value-of select="$created_year"/>
	</xsl:variable>
	<!-- get the page language -->
	<xsl:variable name="lang">
		<xsl:choose><!-- make sure we get some language, we default to English -->
			<xsl:when test="snap/page/body/lang"><xsl:value-of select="snap/page/body/lang"/></xsl:when>
			<xsl:otherwise>en</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>

	<!-- transform the specified path in a full path as required -->
	<xsl:function name="snap:prepend-base" as="xs:string">
		<xsl:param name="website_uri" as="xs:string"/>
		<xsl:param name="base_uri" as="xs:string"/>
		<xsl:param name="path" as="xs:string"/>
		<xsl:variable name="lpath"><xsl:value-of select="lower-case($path)"/></xsl:variable>

		<xsl:choose>
			<xsl:when test="matches($lpath, '^[a-z]+://')">
				<!-- full path, use as is -->
				<xsl:value-of select="$path"/>
			</xsl:when>
			<xsl:when test="starts-with($lpath, '/')">
				<!-- root path, use with website URI -->
				<xsl:value-of select="concat($website_uri, substring($path, 2))"/>
			</xsl:when>
			<xsl:otherwise>
				<!-- relative path, use with base URI -->
				<xsl:value-of select="concat($base_uri, $path)"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:function>

	<xsl:template match="snap">
		<!-- circumvent a QXmlQuery bug, if global variables are not accessed
		     at least once then they may appear as undefined in a function. -->
		<!--xsl:if test="$base_uri != ''"></xsl:if>
		<xsl:if test="$website_uri != ''"></xsl:if-->

		<html lang="{$lang}" xml:lang="{$lang}" prefix="og: http://ogp.me/ns#">
			<head>
				<meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
				<!-- title is required, no need to test its presence -->
				<xsl:variable name="title" select="page/body/titles/title"/>
				<xsl:variable name="site_name" select="head/metadata/desc[@type='name']/data"/>
				<title><xsl:value-of select="$title"/> | <xsl:value-of select="$site_name"/></title>
				<meta property="og:title" content="{$title}"/>
				<meta property="og:site_name" content="{$site_name}"/>
				<meta property="og:type" content="website"/>
				<!-- generator -->
				<link rel="bookmark" type="text/html" title="Generator" href="http://snapwebsites.org/"/>
				<meta name="generator" content="Snap! Websites"/>
				<!-- canonical (must be complete so we do not try to snap:prepend-base()) -->
				<xsl:if test="$page_uri != ''">
					<link rel="canonical" type="text/html" title="Canonical URI" href="{$page_uri}"/>
					<meta property="og:url" content="{$page_uri}"/>
				</xsl:if>
				<!-- include dcterms? -->
				<xsl:if test="$use_dcterms = 'yes'">
					<link rel="schema.dcterms" type="text/uri-list" href="http://purl.org/dc/terms/"/>
				</xsl:if>
				<!-- short description (abstract) -->
				<xsl:if test="page/body/abstract != ''">
					<xsl:variable name="abstract" select="page/body/abstract"/>
					<meta name="abstract" content="{$abstract}"/>
					<xsl:if test="$use_dcterms = 'yes'">
						<meta name="dcterms.abstract" content="{$abstract}"/>
					</xsl:if>
				</xsl:if>
				<!-- long description (description) -->
				<xsl:if test="page/body/description != ''">
					<xsl:variable name="description" select="page/body/description"/>
					<meta name="description" content="{$description}"/>
					<meta property="og:description" content="{$description}"/>
					<xsl:if test="$use_dcterms = 'yes'">
						<meta name="dcterms.description" content="{$description}"/>
					</xsl:if>
				</xsl:if>
				<!-- copyright (by owner) / license / provenance -->
				<xsl:if test="page/body/owner != ''">
					<xsl:variable name="owner" select="page/body/owner"/>
					<meta name="copyright" content="Copyright © {$year_range} by {$owner}"/>
					<xsl:if test="$use_dcterms = 'yes'">
						<meta name="dcterms.rights" content="Copyright © {$year_range} by {$owner}"/>
						<xsl:choose>
							<xsl:when test="page/body/owner[@href] != ''">
								<xsl:variable name="href" select="page/body/owner/@href"/>
								<link rel="dcterms.rightsHolder" type="text/html" title="{$owner}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
							</xsl:when>
							<xsl:otherwise>
								<meta name="dcterms.rightsHolder" content="{$owner}"/>
							</xsl:otherwise>
						</xsl:choose>
						<xsl:if test="page/body/license != ''">
							<xsl:variable name="license" select="page/body/license"/>
							<xsl:choose>
								<xsl:when test="page/body/license[@href] != ''">
									<xsl:variable name="href" select="page/body/license/@href"/>
									<link rel="dcterms.license" type="text/html" title="{$license}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
								</xsl:when>
								<xsl:otherwise>
									<meta name="dcterms.license" content="{$license}"/>
								</xsl:otherwise>
							</xsl:choose>
						</xsl:if>
						<xsl:if test="page/body/provenance != ''">
							<xsl:variable name="provenance" select="page/body/provenance"/>
							<xsl:choose>
								<xsl:when test="page/body/provenance[@href] != ''">
									<xsl:variable name="href" select="page/body/provenance/@href"/>
									<link rel="dcterms.provenance" type="text/html" title="{$provenance}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
								</xsl:when>
								<xsl:otherwise>
									<meta name="dcterms.provenance" content="{$provenance}"/>
								</xsl:otherwise>
							</xsl:choose>
						</xsl:if>
					</xsl:if>
				</xsl:if>
				<!-- publisher -->
				<xsl:if test="$use_dcterms = 'yes'">
					<xsl:if test="page/body/publisher != ''">
						<xsl:variable name="publisher" select="page/body/publisher"/>
						<xsl:choose>
							<xsl:when test="page/body/publisher[@href] != ''">
								<xsl:variable name="href" select="page/body/publisher/@href"/>
								<link rel="dcterms.publisher" type="text/html" title="{$publisher}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
							</xsl:when>
							<xsl:otherwise>
								<meta name="dcterms.publisher" content="{$publisher}"/>
							</xsl:otherwise>
						</xsl:choose>
					</xsl:if>
				</xsl:if>
				<!-- location (including dcterms.coverage) -->
				<xsl:if test="page/body/location/postal-code != ''">
					<xsl:variable name="zipcode" select="page/body/location/postal-code"/>
					<meta name="zipcode" content="{$zipcode}"/>
				</xsl:if>
				<xsl:if test="page/body/location/city != ''">
					<xsl:variable name="city" select="page/body/location/city"/>
					<meta name="city" content="{$city}"/>
				</xsl:if>
				<xsl:if test="page/body/location/country != ''">
					<xsl:variable name="country" select="page/body/location/country"/>
					<meta name="country" content="{$country}"/>
				</xsl:if>
				<xsl:if test="$use_dcterms = 'yes'">
					<!-- jurisdiction -->
					<xsl:if test="page/body/location/jurisdiction != ''">
						<xsl:variable name="jurisdiction" select="page/body/location/jurisdiction"/>
						<xsl:choose>
							<xsl:when test="page/body/location/jurisdiction[@href] != ''">
								<xsl:variable name="href" select="page/body/location/jurisdiction/@href"/>
								<link rel="dcterms.coverage" type="text/html" title="{$jurisdiction}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
							</xsl:when>
							<xsl:otherwise>
								<meta name="dcterms.coverage" content="{$jurisdiction}"/>
							</xsl:otherwise>
						</xsl:choose>
					</xsl:if>
					<!-- longitude,latitude -->
					<xsl:if test="page/body/location/longlat != ''">
						<xsl:variable name="longlat" select="page/body/location/longlat"/>
						<xsl:choose>
							<xsl:when test="page/body/location/longlat[@href] != ''">
								<xsl:variable name="href" select="page/body/location/longlat/@href"/>
								<link rel="dcterms.spacial" type="text/html" title="{$longlat}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
							</xsl:when>
							<xsl:otherwise>
								<meta name="dcterms.spacial" content="{$longlat}"/>
							</xsl:otherwise>
						</xsl:choose>
					</xsl:if>
				</xsl:if>
				<!-- author (one tag per author) -->
				<xsl:for-each select="page/body/author">
					<xsl:variable name="author" select="."/>
					<meta name="author" content="{$author}"/>
					<xsl:if test="$use_dcterms = 'yes'">
						<meta name="dcterms.creator" content="{$author}"/>
					</xsl:if>
					<xsl:if test="@href">
						<xsl:variable name="href" select="@href"/>
						<link rel="author" type="text/html" title="{$author}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
					</xsl:if>
				</xsl:for-each>
				<!-- contributor (one tag per contributor) -->
				<xsl:for-each select="page/body/contributor">
					<xsl:variable name="contributor" select="."/>
					<xsl:if test="$use_dcterms = 'yes'">
						<meta name="dcterms.contributor" content="{$contributor}"/>
					</xsl:if>
					<xsl:if test="@href">
						<xsl:variable name="href" select="@href"/>
						<link rel="contributor" type="text/html" title="{$contributor}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
					</xsl:if>
				</xsl:for-each>
				<!-- language (if not defined in source, no meta tag) -->
				<xsl:if test="page/body/lang != ''">
					<xsl:variable name="language" select="page/body/lang"/>
					<meta name="language" content="{$language}"/>
					<meta property="og:locale" content="{$language}"/>
					<xsl:if test="$use_dcterms = 'yes'">
						<meta name="dcterms.language" content="{$language}"/>
					</xsl:if>
				</xsl:if>
				<!-- dates -->
				<xsl:choose>
					<xsl:when test="page/body/modified">
						<xsl:variable name="modified" select="page/body/modified"/>
						<meta name="date" content="{$modified}"/>
						<xsl:if test="$use_dcterms = 'yes'">
							<meta name="dcterms.date" content="{$modified}"/>
						</xsl:if>
					</xsl:when>
					<xsl:when test="page/body/updated">
						<xsl:variable name="updated" select="page/body/updated"/>
						<meta name="date" content="{$updated}"/>
						<xsl:if test="$use_dcterms = 'yes'">
							<meta name="dcterms.date" content="{$updated}"/>
						</xsl:if>
					</xsl:when>
					<xsl:when test="page/body/created">
						<xsl:variable name="created" select="page/body/created"/>
						<meta name="date" content="{$created}"/>
						<xsl:if test="$use_dcterms = 'yes'">
							<meta name="dcterms.date" content="{$created}"/>
						</xsl:if>
					</xsl:when>
				</xsl:choose>
				<xsl:if test="$use_dcterms = 'yes' and page/body/created != ''">
					<xsl:variable name="created" select="page/body/created"/>
					<meta name="dcterms.created" content="{$created}"/>
				</xsl:if>
				<xsl:if test="$use_dcterms = 'yes' and page/body/since != '' and page/body/until != ''">
					<xsl:variable name="since" select="page/body/since"/>
					<xsl:variable name="until" select="page/body/until"/>
					<xsl:if test="xs:dateTime($since) lt xs:dateTime($until)">
						<meta name="dcterms.valid" content="{$since} {$until}"/>
						<xsl:variable name="extent" select="days-from-duration(xs:dateTime($until) - xs:dateTime($since))"/>
						<xsl:choose>
							<xsl:when test="$extent &lt;= 0"></xsl:when>
							<xsl:when test="$extent = 1"><meta name="dcterms.extent" content="1 day"/></xsl:when>
							<xsl:otherwise><meta name="dcterms.extent" content="{$extent} days"/></xsl:otherwise>
						</xsl:choose>
					</xsl:if>
				</xsl:if>
				<xsl:if test="$use_dcterms = 'yes' and page/body/issued != ''">
					<xsl:variable name="issued" select="page/body/issued"/>
					<meta name="dcterms.issued" content="{$issued}"/>
				</xsl:if>
				<xsl:if test="$use_dcterms = 'yes' and page/body/accepted != ''">
					<xsl:variable name="accepted" select="page/body/accepted"/>
					<meta name="dcterms.dateAccepted" content="{$accepted}"/>
				</xsl:if>
				<xsl:if test="$use_dcterms = 'yes' and page/body/copyrighted != ''">
					<xsl:variable name="copyrighted" select="page/body/copyrighted"/>
					<meta name="dcterms.dateCopyrighted" content="{$copyrighted}"/>
				</xsl:if>
				<xsl:if test="$use_dcterms = 'yes' and page/body/submitted != ''">
					<xsl:variable name="submitted" select="page/body/submitted"/>
					<meta name="dcterms.dateSubmitted" content="{$submitted}"/>
				</xsl:if>
				<!-- robots -->
				<xsl:if test="page/body/robots/tracking">
					<xsl:variable name="tracking" select="page/body/robots/tracking"/>
					<meta name="robots" content="{$tracking}"/>
				</xsl:if>
				<xsl:if test="page/body/robots/changefreq">
					<xsl:variable name="changefreq" select="page/body/robots/changefreq"/>
					<meta name="revisit-after" content="{$changefreq}"/>
				</xsl:if>
				<!-- medium -->
				<xsl:if test="$use_dcterms = 'yes'">
					<xsl:if test="page/body/medium != ''">
						<xsl:variable name="medium" select="page/body/medium"/>
						<meta name="dcterms.medium" content="{$medium}"/>
					</xsl:if>
				</xsl:if>
				<!-- identifier -->
				<xsl:if test="$use_dcterms = 'yes'">
					<xsl:if test="page/body/identifier != ''">
						<xsl:variable name="identifier" select="page/body/identifier"/>
						<meta name="dcterms.identifier" content="{$identifier}"/>
					</xsl:if>
				</xsl:if>
				<!-- instructions -->
				<xsl:if test="$use_dcterms = 'yes' and page/body/instructions">
					<xsl:variable name="instructions" select="page/body/instructions"/>
					<xsl:choose>
						<xsl:when test="page/body/instructions/@href">
							<xsl:variable name="href" select="page/body/instructions/@href"/>
							<link rel="dcterms.instructionalMethod" type="text/html" title="{$instructions}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
						</xsl:when>
						<xsl:otherwise>
							<meta name="dcterms.instructionalMethod" content="{$instructions}"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:if>
				<!-- permission -->
				<xsl:if test="$use_dcterms = 'yes' and page/body/permission">
					<xsl:variable name="permission" select="page/body/permission"/>
					<xsl:choose>
						<xsl:when test="page/body/permission/@href">
							<xsl:variable name="href" select="page/body/permission/@href"/>
							<link rel="dcterms.accessRights" type="text/html" title="{$permission}" href="{snap:prepend-base($website_uri, $base_uri, $href)}"/>
						</xsl:when>
						<xsl:otherwise>
							<meta name="dcterms.accessRights" content="{$permission}"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:if>
				<!-- shortcut icon -->
				<xsl:for-each select="page/body/image/shortcut">
					<xsl:call-template name="shortcut-link">
						<xsl:with-param name="type" select="@type"/>
						<xsl:with-param name="href" select="@href"/>
						<xsl:with-param name="width" select="@width"/>
						<xsl:with-param name="height" select="@height"/>
					</xsl:call-template>
				</xsl:for-each>
				<!-- main image -->
				<xsl:if test="page/body/image">
					<xsl:variable name="image_id" select="page/body/image/@idref"/>
					<!-- should it be content or output? -->
					<xsl:for-each select="page/body/content/descendant::img[@id=$image_id][1]">
						<!-- note: we expect just one entry, we still use [1] to make sure -->
						<xsl:variable name="src" select="@src"/>
						<meta property="og:image" content="{snap:prepend-base($website_uri, $base_uri, $src)}"/>
					</xsl:for-each>
				</xsl:if>
				<!-- navigation -->
				<xsl:if test="page/body/navigation[@href]">
					<xsl:variable name="sitemap" select="page/body/navigation/@href"/>
					<link rel="sitemap" type="text/html" title="Sitemap" href="{$sitemap}"/>
				</xsl:if>
				<xsl:if test="page/body/toc[@href]">
					<xsl:variable name="toc" select="page/body/toc/@href"/>
					<link rel="toc" type="text/html" title="toc" href="{$toc}"/>
				</xsl:if>
				<xsl:for-each select="page/body/navigation/link">
					<xsl:variable name="rel" select="@rel"/>
					<xsl:variable name="href" select="@href"/>
					<link rel="{$rel}" type="text/html" href="{$href}"/>
				</xsl:for-each>
				<!-- bookmarks -->
				<xsl:for-each select="page/body/bookmarks/link">
					<xsl:call-template name="bookmark-link">
						<xsl:with-param name="rel" select="@rel"/>
						<xsl:with-param name="href" select="@href"/>
						<xsl:with-param name="title" select="@title"/>
					</xsl:call-template>
				</xsl:for-each>
				<xsl:if test="page/body/bookmarks[@href]">
					<xsl:variable name="sitemap" select="page/body/navigation/@href"/>
					<link rel="sitemap" type="text/html" title="Sitemap" href="{$sitemap}"/>
				</xsl:if>
				<!-- table of contents -->
				<xsl:for-each select="page/body/toc/entry">
					<xsl:call-template name="toc-link">
						<xsl:with-param name="rel" select="@element"/>
						<xsl:with-param name="href" select="@href"/>
						<xsl:with-param name="title" select="."/>
					</xsl:call-template>
				</xsl:for-each>
				<!-- translations -->
				<xsl:if test="page/body/translations">
					<xsl:variable name="mode" select="page/body/translations/@mode"/>
					<xsl:for-each select="page/body/translations/l">
						<xsl:call-template name="alternate-language">
							<xsl:with-param name="mode" select="$mode"/>
							<xsl:with-param name="lang" select="@lang"/>
							<xsl:with-param name="title" select="."/>
						</xsl:call-template>
					</xsl:for-each>
				</xsl:if>
				<!-- formats -->
				<xsl:for-each select="page/body/formats/f">
					<xsl:call-template name="alternate-format">
						<xsl:with-param name="format" select="@format"/>
						<xsl:with-param name="type" select="@type"/>
						<xsl:with-param name="title" select="."/>
					</xsl:call-template>
				</xsl:for-each>
				<!-- apply user defined meta tags -->
				<xsl:for-each select="head/metadata/desc[@type='user']">
					<xsl:variable name="name" select="@name"/>
					<xsl:variable name="content" select="data"/>
					<meta name="{$name}" content="{$content}"/>
				</xsl:for-each>
		<style>
body
{
	margin: 0px;
	padding: 0px;
	background-color: white;
	font: 10px sans-serif;
}

#page
{
	border-top: 1px #cccccc solid;
	background-color: #cccccc;
}

#header
{
}

#page-top-wrapper
{
	width: 850px;
	margin: 0px auto;
	background-color: red; /*white;*/
	border: 1px black solid;
	min-height: 45px;
}

#page-top-left
{
	padding-left: 8px;
	background: white url(http://linux.m2osw.com/sites/all/themes/white/images/top-left.png) no-repeat 0% 0%;
}

#page-top-right
{
	padding-right: 8px;
	background: white url(http://linux.m2osw.com/sites/all/themes/white/images/top-right.png) no-repeat 100% 0%;
}

#page-top
{
	padding-top: 10px;
	padding-right: 10px;
	background-color: white;
}

#page-top-alignment
{
	float: right;
}

#page-top-alignment div
{
	float: left;
}

.white-clear
{
	clear: both;
	height: 0px;
}

#page-bottom-color
{
	background-color: #888888;
}

#page-bottom-wrapper
{
	width: 850px;
	margin: 0px auto;
	background-color: white;
	border-top: 1px white solid;
}

#page-bottom-left
{
	padding-left: 8px;
	background: white url(http://linux.m2osw.com/sites/all/themes/white/images/bottom-left.png) no-repeat 0% 100%;
}

#page-bottom-right
{
	padding-right: 8px;
	background: white url(http://linux.m2osw.com/sites/all/themes/white/images/bottom-right.png) no-repeat 100% 100%;
}

#page-bottom
{
	background-color: white;
	min-height: 85px;
	text-align: center;
}

#white-footer
{
	clear: both;
	text-align: center;
	color: white;
	background-color: #888888;
	border-top: 1px #888888 solid;
}

#node
{
	min-height: 600px;
	background: #888888 url(http://linux.m2osw.com/sites/all/themes/white/images/background.png) repeat-x 0 0;
}

#wrapper
{
	width: 850px;
	margin: -10px auto 0px;
	padding: 0px;
	background-color: white;
}
		</style>
			</head>
			<xsl:apply-templates select="page/body"/>
		</html>
	</xsl:template>
	<xsl:template name="bookmark-link">
		<xsl:param name="rel"/>
		<xsl:param name="href"/>
		<xsl:param name="title"/>
		<xsl:choose>
			<xsl:when test="$title">
				<link rel="{$rel}" type="text/html" title="{$title}" href="{$href}"/>
			</xsl:when>
			<xsl:otherwise>
				<link rel="{$rel}" type="text/html" href="{$href}"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	<xsl:template name="alternate-language">
		<xsl:param name="mode"/>
		<xsl:param name="lang"/>
		<xsl:param name="title"/>
		<xsl:choose>
			<xsl:when test="$mode = 'sub-domain'">
				<link rel="alternate" type="text/html" title="{$title}" hreflang="{$lang}" href="{concat($protocol, '://', $lang, '.', $domain, '/', $full_path)}"/>
			</xsl:when>
			<xsl:when test="$mode = 'extension'">
				<link rel="alternate" type="text/html" title="{$title}" hreflang="{$lang}" href="{concat($page_uri, '.', $lang)}"/>
			</xsl:when>
			<xsl:otherwise><!-- mode = 'path' -->
				<link rel="alternate" type="text/html" title="{$title}" hreflang="{$lang}" href="{concat($website_uri, $lang, '/', $path, $page)}"/>
			</xsl:otherwise>
		</xsl:choose>
		<meta property="og:locale:alternate" content="{$lang}"/>
	</xsl:template>
	<xsl:template name="alternate-format">
		<xsl:param name="format"/>
		<xsl:param name="type"/>
		<xsl:param name="title"/>
		<link rel="alternate" type="{$type}" title="{$title}" href="{concat($page_uri, '.', $format)}"/>
	</xsl:template>
	<xsl:template name="shortcut-link">
		<xsl:param name="type"/>
		<xsl:param name="href"/>
		<xsl:param name="width"/>
		<xsl:param name="height"/>
		<xsl:if test="$width = 16 and $height = 16">
			<link rel="shortcut" type="{$type}" href="{$href}"/>
		</xsl:if>
		<xsl:if test="$width > 16 and $height > 16">
			<link rel="apple-touch-icon" type="{$type}" sizes="{concat($width, 'x', $height)}" href="{$href}"/>
		</xsl:if>
	</xsl:template>
	<xsl:template name="toc-link">
		<xsl:param name="rel"/>
		<xsl:param name="href"/>
		<xsl:param name="title"/>
		<link rel="{$rel}" type="text/html" title="{$title}" href="{$href}"/>
	</xsl:template>
	<!--xsl:template match="head">
		<head>Nothing</head>
	</xsl:template-->
	<xsl:template match="page/body">
		<body>
      <xsl:call-template name="snap:user-messages"/>
			<div id="page">
				<div id="header">Header...</div>
				<div id="page-top-wrapper">
					<div id="page-top-left">
						<div id="page-top-right">
							<div id="page-top">
								<div id="page-top-alignment">
									Something
								</div>
							</div>
						</div>
					</div>
					<div class="white-clear"></div>
					End top
				</div>
				<div id="node">
					Here Now
					<div id="wrapper">

									<h1>White Theme</h1>
									<xsl:copy-of select="output/*"/>

					</div>
				</div>
				<div id="page-bottom-color">
					<div id="page-bottom-wrapper">
						<div id="page-bottom-left">
							<div id="page-bottom-right">
								<div id="page-bottom">
								</div>
							</div>
						</div>
					</div><!-- page-bottom-wrapper -->
					<div class="white-clear"></div>
					<div id="white-footer">
					</div>
				</div>
			</div>
		</body>
	</xsl:template>
	<xsl:template name="left">
		<xsl:apply-templates>
				<!-- this or the following both work:
				 xsl:sort select="@priority" data-type="number" /-->
				<xsl:sort select="xs:decimal(@priority)"/>
		</xsl:apply-templates>
	</xsl:template>
	<xsl:template name="left-boxes">
		<!-- left area -->
		<div>
			<xsl:attribute name="class">left area</xsl:attribute>
			<div class="box">Box[<xsl:value-of select="position()"/>] = (<xsl:value-of select="@priority"/>) <xsl:value-of select="title"/></div>
		</div>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
