<?xml version="1.0"?>
<!--
Snap Websites Server == plugin selection parser
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

	<xsl:template match="snap">
		<output> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div>
				<xsl:attribute name="class">plugin<xsl:if test="core-plugin/node() = 'true'"> core-plugin</xsl:if></xsl:attribute>
				<xsl:attribute name="data-name"><xsl:value-of select="name/node()"/></xsl:attribute>
				<div class="top-row">
					<div class="name">
						<xsl:attribute name="id"><xsl:value-of select="name/node()"/></xsl:attribute>
						<h3>
							<xsl:if test="installed/node() = 'true' or core-plugin/node() = 'true'">
								<xsl:attribute name="class">installed</xsl:attribute>
								<xsl:attribute name="title">Installed</xsl:attribute>
							</xsl:if>
							<span class="installed-checkmark"><xsl:if test="installed/node() = 'true' or core-plugin/node() = 'true'">&#10004;&#160;</xsl:if></span>
							<xsl:copy-of select="name/node()"/>
							v<xsl:copy-of select="version-major/node()"/>.<xsl:copy-of select="version-minor/node()"/>
							<xsl:if test="core-plugin/node() = 'true'"> (Core)</xsl:if>
						</h3>
					</div>
					<ul class="functions">
						<xsl:if test="not(@locked)">
							<li>
								<a href="#" data-plugin-name="{name/node()}">
									<xsl:attribute name="class">
										button install
										<xsl:if test="installed/node() = 'true' or core-plugin/node() = 'true'">
											disabled
										</xsl:if>
									</xsl:attribute>
									Install
								</a>
							</li>
							<li>
								<a href="#" data-plugin-name="{name/node()}">
									<xsl:attribute name="class">
										button remove
										<xsl:if test="installed/node() = 'false' or core-plugin/node() = 'true'">
											disabled
										</xsl:if>
									</xsl:attribute>
									Remove
								</a>
							</li>
						</xsl:if>
						<li>
							<xsl:choose>
								<xsl:when test="settings-path = ''">
									<!-- no settings at all, we can just ignore this one completely -->
									<span class="button disabled">Settings</span>
								</xsl:when>
								<xsl:otherwise>
									<a href="/{settings-path}" data-plugin-name="{name/node()}">
										<xsl:attribute name="class">
											button settings
											<xsl:if test="installed/node() = 'false'">disabled</xsl:if>
										</xsl:attribute>
										Settings
									</a>
								</xsl:otherwise>
							</xsl:choose>
						</li>
						<li><a href="{help-uri}" target="_blank" class="button">Help</a></li>
					</ul>
				</div>
				<div class="description-row">
					<div class="icon"><img src="{icon/node()}"/></div>
					<div class="description"><xsl:copy-of select="description/node()"/></div>
				</div>
				<div class="last-row">
					<!-- advance information -->
					<div class="last-modification"><strong>Last modified on: </strong> <span class="date"><xsl:copy-of select="last-modification-date/node()"/></span></div>
					<div class="last-update">
						<xsl:choose>
							<xsl:when test="last-updated-date and last-updated-date != ''">
								<strong>Last updated on: </strong> <span class="date"><xsl:copy-of select="last-updated-date/node()"/></span>
							</xsl:when>
							<xsl:otherwise>
								<strong>Never updated</strong>
							</xsl:otherwise>
						</xsl:choose>
					</div>
					<div class="filename"><strong>Loading from: </strong> <span class="path"><xsl:copy-of select="filename/node()"/></span></div>
					<div class="dependencies"><strong>Dependencies: </strong>
						<xsl:choose>
							<xsl:when test="dependencies/node()">
								<span class="plugin-names">
									<xsl:for-each select="tokenize(dependencies/node(), ',')">
										<xsl:if test="position() != 1">, </xsl:if>
										<a href="#{.}"><xsl:value-of select="."/></a>
									</xsl:for-each>
								</span>
							</xsl:when>
							<xsl:otherwise>
								None
							</xsl:otherwise>
						</xsl:choose>
					</div>
				</div>
			</div>

			<javascript name="plugin-selection"/>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
