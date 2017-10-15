<?xml version="1.0"?>
<!--
Snap Websites Server == timetracker day form parser
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
	<xsl:variable name="layout-area">timetracker-parser</xsl:variable>
	<xsl:variable name="layout-modified">2016-01-02 05:32:33</xsl:variable>
	<xsl:variable name="layout-editor">timetracker-page</xsl:variable>

	<xsl:template match="snap">
		<output> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form timetracker-day-form" form_name="timetracker">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>

				<div>

						<!-- row -->
						<div class="editor-block">
							<label for="client" class="editor-title">Client: <span class="required">*</span></label>
							<xsl:copy-of select="page/body/timetracker/client/node()"/>
						</div>
						<div class="editor-block">
							<label for="billing_duration" class="editor-title">Billing Duration: <span class="required">*</span></label>
							<xsl:copy-of select="page/body/timetracker/billing_duration/node()"/>
						</div>
						<div class="editor-block">
							<label for="location" class="editor-title">Location: <span class="required">*</span></label>
							<xsl:copy-of select="page/body/timetracker/location/node()"/>
							<label for="transportation" class="editor-title">Transportation:</label>
							<xsl:copy-of select="page/body/timetracker/transportation/node()"/>
						</div>
						<div class="editor-block">
							<label for="description" class="editor-title">Description:</label>
							<xsl:copy-of select="page/body/timetracker/description/node()"/>
						</div>

				</div>

				<div class="buttons">
					<a class="timetracker-button cancel-button" href="#cancel">Cancel</a>
					<a class="timetracker-button save-button editor-default-button" href="#save">Save</a>
				</div>

			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
