<?xml version="1.0"?>
<!--
Snap Websites Server == editor test parser for our test form
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
	<xsl:variable name="layout-area">info-settings-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-09-07 23:22:10</xsl:variable>
	<xsl:variable name="layout-editor">info-settings-page</xsl:variable>

	<xsl:template match="snap">
		<output><!-- lang="{$lang}"-->
			<div class="editor-form" form_name="favicon">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset class="small-icon">
						<legend>Favority Icons</legend>

						<!-- site name widget -->
						<div class="editor-block">
							<label for="site_name_widget" class="editor-title">Small Icon:</label>
							<xsl:copy-of select="page/body/favicon/icon/node()"/>
						</div>

						<!-- list of default .ico files one can use -->
						<!-- TODO: look into a way to this list dynamically growable -->
						<div class="editor-block">
							<ul class="default-icons">
								<li><a href="#asterisk"><img src="/images/favicon/asterisk.ico" title="Asterisk"/></a></li>
								<li><a href="#checkmark"><img src="/images/favicon/checkmark.ico" title="Checkmark"/></a></li>
								<li><a href="#earth"><img src="/images/favicon/earth.ico" title="Earth"/></a></li>
								<li><a href="#five-colors"><img src="/images/favicon/five-colors.ico" title="Five Colors"/></a></li>
								<li><a href="#ladybug"><img src="/images/favicon/ladybug.ico" title="Ladybug"/></a></li>
								<li><a href="#molecule"><img src="/images/favicon/molecule.ico" title="Molecule"/></a></li>
								<li><a href="#ring"><img src="/images/favicon/ring.ico" title="Ring"/></a></li>
								<li><a href="#snap-favicon"><img src="/images/favicon/snap-favicon.ico" title="Snap"/></a></li>
								<li><a href="#star"><img src="/images/favicon/star.ico" title="Star"/></a></li>
								<li><a href="#yellow-ball"><img src="/images/favicon/yellow-ball.ico" title="Yellow Ball"/></a></li>
							</ul>
						</div>

					</fieldset>

				</div>
			</div>

			<javascript name="favicon"/>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
