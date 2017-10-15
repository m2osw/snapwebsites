<?xml version="1.0"?>
<!--
Snap Websites Server == system messages XSLT templates
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

	<!-- *** INTERNAL TEMPLATES *** -->

	<!-- handle the shortcut icon link -->
	<xsl:template name="shortcut-link">
		<xsl:param name="type"/>
		<xsl:param name="href"/>
		<xsl:param name="width"/>
		<xsl:param name="height"/>
		<xsl:if test="$width &lt;= 16 and $height &lt;= 16">
			<!--
					the proper (per spec.) rel token is rel="icon", and the type
					is "image/vnd.microsoft.icon"; however, these are only supported
					in IE 11+ and since that's still the majority of people who use
					that broken system better support its quirks...
			-->
			<!-- link rel="icon" type="image/vnd.microsoft.icon" href="{$href}"/ -->
			<link rel="shortcut icon" type="{$type}" href="{$href}"/>
		</xsl:if>
		<xsl:if test="$width &gt; 16 and $height &gt; 16">
			<link rel="apple-touch-icon" type="{$type}" sizes="{concat($width, 'x', $height)}" href="{$href}"/>
			<!--
					according to this bug report
						https://bugzilla.mozilla.org/show_bug.cgi?id=751712
					firefox "icon" supports sizes up to 196x196

					only we would have to make sure that the last entry is the correct
					one as it seems that firefox uses the last and not the closest
					available size

					see also:
						http://stackoverflow.com/questions/2268204/favicon-dimensions
			-->
			<link rel="icon" type="{$type}" sizes="{concat($width, 'x', $height)}" href="{$href}"/>
	 </xsl:if>
	</xsl:template>

	<!-- handle one bookmark link -->
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

	<!-- handle one table of content link -->
	<xsl:template name="toc-link">
		<xsl:param name="rel"/>
		<xsl:param name="href"/>
		<xsl:param name="title"/>
		<link rel="{$rel}" type="text/html" title="{$title}" href="{$href}"/>
	</xsl:template>

	<!-- handle one alternate language -->
	<xsl:template name="alternate-language">
		<xsl:param name="mode"/>
		<xsl:param name="lang"/>
		<xsl:param name="title"/>
		<xsl:choose>
			<!-- TODO: the name of the query string can be changed -->
			<xsl:when test="$mode = 'query-string'">
				<link rel="alternate" type="text/html" title="{$title}" hreflang="{$lang}" href="{concat($page_uri, '?lang=', $lang)}"/>
			</xsl:when>
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

	<!-- handle one alternate format -->
	<xsl:template name="alternate-format">
		<xsl:param name="href"/>
		<xsl:param name="type"/>
		<xsl:param name="title"/>
		<xsl:variable name="sub-title">
			<xsl:choose>
				<xsl:when test="$type = 'application/atom+xml'"> (Atom)</xsl:when>
				<xsl:when test="$type = 'application/rss+xml'"> (RSS)</xsl:when>
			</xsl:choose>
		</xsl:variable>
		<link rel="alternate" type="{$type}" title="{$title}{$sub-title}" href="{$href}"/>
	</xsl:template>


	<!-- *** PUBLIC HEADER TEMPLATE *** -->

	<xsl:template name="snap:html-header">
		<xsl:param name="theme-css" select="''"/>

		<!-- correct charset in HTML5 -->
		<meta charset="utf-8"/>
		<!-- force UTF-8 encoding (at the very beginning to avoid an IE6 bug) -->
		<meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
		<!-- viewport for smart phones support (TODO: we need to have some support
		     in the header plugin to tweak the values...) -->
		<meta name="viewport" content="width=device-width, initial-scale=1"/>

		<!-- title is required, no need to test its presence -->
		<xsl:variable name="title" select="page/body/titles/title/node()"/>
		<xsl:variable name="site_name" select="head/metadata/desc[@type='name']/data"/>
		<title><xsl:value-of select="$title"/> | <xsl:value-of select="$site_name"/></title>
		<meta property="og:title" content="{$title}"/>
		<meta property="og:site_name" content="{$site_name}"/>
		<meta property="og:type" content="website"/>

		<!-- generator -->
		<link rel="bookmark" type="text/html" title="Generator" href="http://snapwebsites.org/"/>
		<meta name="generator">
			<xsl:attribute name="content">Snap! Websites v<xsl:value-of select="head/metadata/desc[@type='version']/data"/></xsl:attribute>
		</meta>
		<meta name="application-name" content="snapwebsites"/>

		<!-- canonical -->
		<xsl:variable name="canonical_uri" as="xs:string" select="/snap/head/metadata/desc[@type='canonical_uri']/data"/>
		<xsl:if test="$canonical_uri != ''">
			<link rel="canonical" type="text/html" title="{$title}" href="{$canonical_uri}"/>
			<meta property="og:url" content="{$canonical_uri}"/>
		</xsl:if>

		<!-- URI of this page -->
		<xsl:if test="$page_uri != ''">
			<link rel="page-uri" type="text/html" title="Site URI" href="{$page_uri}"/>
		</xsl:if>

		<!-- shortlink == this is where we could test having or not having a plugin could dynamically change the XSL templates -->
		<xsl:variable name="shorturl" select="head/metadata/desc[@type='shorturl']/data"/>
		<xsl:if test="$shorturl != ''">
			<link rel="shortlink" type="text/html" title="Short Link" href="{$shorturl}"/>
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
			<meta name="copyright" content="Copyright &#xA9; {$year_range} by {$owner}"/>
			<xsl:if test="$use_dcterms = 'yes'">
				<meta name="dcterms.rights" content="Copyright &#xA9; {$year_range} by {$owner}"/>
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
				<meta name="date" content="{page/body/modified}"/>
				<xsl:if test="$use_dcterms = 'yes'">
					<meta name="dcterms.date" content="{page/body/modified-precise}"/>
				</xsl:if>
			</xsl:when>
			<xsl:when test="page/body/updated">
				<xsl:variable name="updated" select="page/body/updated"/>
				<meta name="date" content="{page/body/updated}"/>
				<xsl:if test="$use_dcterms = 'yes'">
					<meta name="dcterms.date" content="{page/body/updated-precise}"/>
				</xsl:if>
			</xsl:when>
			<xsl:when test="page/body/created">
				<meta name="date" content="{page/body/created}"/>
				<xsl:if test="$use_dcterms = 'yes'">
					<meta name="dcterms.date" content="{page/body/created-precise}"/>
				</xsl:if>
			</xsl:when>
		</xsl:choose>
		<xsl:if test="$use_dcterms = 'yes' and page/body/created-precise != ''">
			<meta name="dcterms.created" content="{page/body/created-precise}"/>
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
		<meta name="msapplication-config" content="none"/>

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
			<xsl:variable name="sitemap" select="page/body/bookmarks/@href"/>
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
		<xsl:for-each select="page/body/formats">
			<xsl:call-template name="alternate-format">
				<xsl:with-param name="href" select="@href"/>
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

		<!-- editor may add a session ID that we stick in a meta tag -->
		<!-- TODO: move that to an editor specific file... -->
		<xsl:if test="head/metadata/editor/session">
			<meta name="editor_session" content="{head/metadata/editor/session}"/>
		</xsl:if>
		<meta name="user_status" content="{substring-after(head/metadata/desc[@type='login_status']/data, 'status::')}"/>
		<meta name="action" content="{$action}"/>
		<meta name="path" content="{$path}"/>
		<meta name="full_path" content="{$full_path}"/>
		<!--meta name="base_uri" content="{$base_uri}"/>
		<meta name="website_uri" content="{$website_uri}"/-->

		<!-- cascading style sheets -->
		<xsl:copy-of select="head/metadata/css/*"/>

		<xsl:if test="$theme-css">
			<link rel="stylesheet" type="text/css" href="{$theme-css}"/>
		</xsl:if>

		<!-- Inline JavaScript Code -->
		<xsl:copy-of select="head/metadata/inline-javascript/*"/>

		<!-- JavaScripts -->
		<xsl:copy-of select="head/metadata/javascript/*"/>

	</xsl:template>


	<!-- *** BODY ATTRIBUTES DEFINED BY THE SYSTEM/PLUGINS *** -->

	<xsl:template name="snap:body-attributes">
		<xsl:attribute name="class"><xsl:value-of
			select="substring-after(/snap/head/metadata/desc[@type='login_status']/data, 'status::')"/>

			<!-- TODO: how do we move that one as part of the editor? -->
			<xsl:if test="/snap/head/metadata/editor[@darken-on-save = 'yes']"> editor-darken-on-save</xsl:if>
		</xsl:attribute>
		<xsl:if test="/snap/head/metadata/editor[@owner != '']">
			<xsl:attribute name="data-editor-form-owner"><xsl:value-of select="/snap/head/metadata/editor/@owner"/></xsl:attribute>
		</xsl:if>
		<xsl:if test="/snap/head/metadata/editor[@id != '']">
			<xsl:attribute name="data-editor-form-id"><xsl:value-of select="/snap/head/metadata/editor/@id"/></xsl:attribute>
		</xsl:if>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
