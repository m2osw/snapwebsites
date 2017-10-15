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
	<xsl:variable name="layout-area">editor-test-parser</xsl:variable>
	<xsl:variable name="layout-modified">2014-10-06 23:14:10</xsl:variable>

	<xsl:template match="snap">
		<output><!-- lang="{$lang}"-->
			<div class="editor-form" form_name="editor_test">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<!-- xsl:if test="$action != 'edit' and $can_edit = 'yes'">
					<a class="settings-edit-button" href="?a=edit">Edit</a>
				</xsl:if>
				<xsl:if test="$action = 'edit'">
					<a class="settings-save-button" href="#">Save Changes</a>
					<a class="settings-cancel-button right-aligned" href="{/snap/head/metadata/desc[@type='page_uri']/data}">Cancel</a>
				</xsl:if-->
				<h2>Widgets of the Snap! WYSIWYG Editor</h2>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset class="drag-and-drop">
						<legend>Drag &amp; Drop Files</legend>

						<!-- file widget -->
						<div class="widget-block">
							<!-- area accepting the drag & drop of an image -->
							<label for="file_widget" class="editor-title">File Widget</label>
							<xsl:copy-of select="page/body/editor/file_widget/node()"/>
						</div>

						<!-- picture widget -->
						<div class="widget-block">
							<!-- area accepting the drag & drop of an image -->
							<label for="picture_widget" class="editor-title">Picture Widget</label>
							<xsl:copy-of select="page/body/editor/picture_widget/node()"/>
						</div>

					</fieldset>

					<fieldset class="text">
						<legend>Text</legend>

						<!-- line edit widget -->
						<div class="editor-block">
							<label for="line_edit_widget" class="editor-title">Line Edit Widget <span class="required">*</span></label>
							<xsl:copy-of select="page/body/editor/line_edit_widget/node()"/>
						</div>

						<!-- password widget -->
						<div class="editor-block">
							<label for="password_widget" class="editor-title">Password Widget <span class="required">*</span></label>
							<xsl:copy-of select="page/body/editor/password_widget/node()"/>
						</div>

						<!-- password/confirm widget -->
						<fieldset class="selection">
							<legend>Password/Confirm</legend>
							<div class="editor-block">
								<xsl:copy-of select="page/body/editor/password_confirm_widget/node()"/>
							</div>
						</fieldset>

						<!-- text edit widget -->
						<div class="editor-block">
							<label for="text_edit_widget" class="editor-title">Text Edit Widget</label>
							<xsl:copy-of select="page/body/editor/text_edit_widget/node()"/>
						</div>

					</fieldset>

					<fieldset class="selection">
						<legend>Selection</legend>

						<!-- dropdown widget -->
						<div class="editor-block">
							<label for="dropdown_widget" class="editor-title">Dropdown Widget <span class="required">*</span></label>
							<xsl:copy-of select="page/body/editor/dropdown_widget/node()"/>
						</div>

						<!-- checkmark widget -->
						<div class="editor-block">
							<label for="checkmark_widget" class="editor-title">Checkmark Widget</label>
							<xsl:copy-of select="page/body/editor/checkmark_widget/node()"/>
						</div>

						<!-- radio widget -->
						<div class="editor-block">
							<label for="radio_widget" class="editor-title">Radio Widget</label>
							<xsl:copy-of select="page/body/editor/radio_widget/node()"/>
						</div>

					</fieldset>

					<fieldset class="buttons">
						<legend>Buttons</legend>

						<div class="editor-block">
							<xsl:copy-of select="page/body/editor/button_widget/node()"/>
						</div>

					</fieldset>

				</div>
			</div>

			<!-- javascript name="editor-test"/ -->
			<!--css name="editor"/-->
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
