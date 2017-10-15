<?xml version="1.0"?>
<!--
Snap Websites Server == editor form for the main feed settings
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
	<xsl:variable name="layout-area">feed-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-10-24 16:28:15</xsl:variable>
	<xsl:variable name="layout-editor">feed-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output><!-- lang="{$lang}"-->
			<div class="editor-form" form_name="feed">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h3>Feed Settings</h3>
				<div>
					<fieldset class="site-wide-feed-parameters">
						<legend>Site Wide Feed Parameters</legend>

						<!-- default logo (channel image) -->
						<div class="editor-block">
							<div class="editor-title">Default Logo (a square for a valid Atom feed, 144x400 max. for a valid RSS feed):</div>
							<xsl:copy-of select="page/body/feed/default_logo/node()"/>
						</div>

						<!-- site long name widget -->
						<div class="editor-block">
							<label for="top_maximum_number_of_items_in_any_feed" class="editor-title">Top maximum number of items in any feed:</label>
							<xsl:copy-of select="page/body/feed/top_maximum_number_of_items_in_any_feed/node()"/>
						</div>
					</fieldset>

					<fieldset class="internal-redirect">
						<legend>Top RSS &amp; Atom</legend>

						<!-- allow access to /rss.xml -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/feed/allow_main_rss_xml/node()"/>
						</div>

						<!-- allow access to /atom.xml -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/feed/allow_main_atom_xml/node()"/>
						</div>
					</fieldset>

					<fieldset class="teaser-parameters">
						<legend>Teaser Parameters</legend>

						<!-- whether to publish the entire article or just a teaser -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/feed/publish_full_article/node()"/>
						</div>

						<!-- define the maximum number of words to include in the teaser -->
						<div class="editor-block">
							<label for="teaser_words" class="editor-title">Maximum number of words in teaser:</label>
							<xsl:copy-of select="page/body/feed/teaser_words/node()"/>
						</div>

						<!-- define the maximum number of tags to include in the teaser -->
						<div class="editor-block">
							<label for="teaser_tags" class="editor-title">Maximum number of tags in teaser:</label>
							<xsl:copy-of select="page/body/feed/teaser_tags/node()"/>
						</div>

						<!-- string to add at the end of the teaser with we reduce the body, may be the empty string -->
						<div class="editor-block">
							<label for="teaser_end_marker" class="editor-title">Teaser end marker (i.e. <strong>[...]</strong>):</label>
							<xsl:copy-of select="page/body/feed/teaser_end_marker/node()"/>
						</div>
					</fieldset>

					<p><a href="http://feedvalidator.org/check.cgi">Validate Your Feed</a> &#x2014; click here to validate RSS and Atom feeds</p>
				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
