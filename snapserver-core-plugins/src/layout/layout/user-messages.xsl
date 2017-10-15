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

	<!-- internal template to display one message -->
	<xsl:template name="snap:message">
		<xsl:param name="type"/>
		<xsl:param name="id"/>
		<xsl:param name="title"/>
		<xsl:param name="body"/>
		<div id="{$id}">
			<xsl:attribute name="class">message message-<xsl:value-of
						select="$type"/><xsl:if test="not($body)"> message-<xsl:value-of
						select="$type"/>-title-only</xsl:if></xsl:attribute>
			<h3><xsl:copy-of select="$title"/></h3>
			<xsl:if test="$body">
				<p><xsl:copy-of select="$body"/></p>
			</xsl:if>
		</div>
	</xsl:template>

	<!-- public template used to display all the user messages -->
	<xsl:template name="snap:user-messages">
		<xsl:if test="/snap/page/body/messages">
			<div class="user-messages">
				<!--div class="user-message-margin"></div-->
				<div>
					<xsl:attribute name="class">user-message-box zordered<xsl:if
							test="/snap/page/body/messages[@error-count != '0']"> error-messages</xsl:if><xsl:if
							test="/snap/page/body/messages[@warning-count != '0']"> warning-messages</xsl:if></xsl:attribute>
					<div class="close-button"><img src="/images/snap/close-button.png"/></div>
					<xsl:for-each select="/snap/page/body/messages/message">
						<xsl:call-template name="snap:message">
							<xsl:with-param name="type" select="@type"/>
							<xsl:with-param name="id" select="@id"/>
							<xsl:with-param name="title" select="title/*"/>
							<xsl:with-param name="body" select="body/*"/>
						</xsl:call-template>
					</xsl:for-each>
				</div>
			</div>
		</xsl:if>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
