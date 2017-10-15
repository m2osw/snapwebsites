<?xml version="1.0"?>
<!--
Snap Websites Server == password policy settings parser
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
	<xsl:variable name="layout-area">password-policy-parser</xsl:variable>
	<xsl:variable name="layout-modified">2015-12-20 23:22:10</xsl:variable>
	<xsl:variable name="layout-editor">password-policy-page</xsl:variable>

	<xsl:template match="snap">
		<output filter="token"> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div class="editor-form" form_name="password">
				<xsl:attribute name="session"><xsl:value-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>
				<xsl:attribute name="timeout"><xsl:value-of select="page/body/editor/timeout/div/div/node()"/></xsl:attribute>

				<h3>Password Policy</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<fieldset class="site-name">
						<legend>Minimum Counts</legend>

						<!-- minimum length widget -->
						<div class="editor-block">
							<label for="minimum_length" class="editor-title">Overall Minimum Length:</label>
							<xsl:copy-of select="page/body/password/minimum_length/node()"/>
						</div>

						<!-- minimum lowercase letters widget -->
						<div class="editor-block">
							<label for="minimum_lowercase_letters" class="editor-title">Minimum Lowercase Letters:</label>
							<xsl:copy-of select="page/body/password/minimum_lowercase_letters/node()"/>
						</div>

						<!-- minimum uppercase letters widget -->
						<div class="editor-block">
							<label for="minimum_uppercase_letters" class="editor-title">Minimum Uppercase Letters:</label>
							<xsl:copy-of select="page/body/password/minimum_uppercase_letters/node()"/>
						</div>

						<!-- minimum letters widget -->
						<div class="editor-block">
							<label for="minimum_letters" class="editor-title">Minimum Letters:</label>
							<xsl:copy-of select="page/body/password/minimum_letters/node()"/>
						</div>

						<!-- minimum digits widget -->
						<div class="editor-block">
							<label for="minimum_digits" class="editor-title">Minimum Digits:</label>
							<xsl:copy-of select="page/body/password/minimum_digits/node()"/>
						</div>

						<!-- minimum spaces widget -->
						<div class="editor-block">
							<label for="minimum_spaces" class="editor-title">Minimum Spaces:</label>
							<xsl:copy-of select="page/body/password/minimum_spaces/node()"/>
						</div>

						<!-- minimum special characters widget -->
						<div class="editor-block">
							<label for="minimum_specials" class="editor-title">Minimum Special Characters:</label>
							<xsl:copy-of select="page/body/password/minimum_specials/node()"/>
						</div>

						<!-- minimum unicode widget -->
						<div class="editor-block">
							<label for="minimum_unicode" class="editor-title">Minimum Unicode Characters:</label>
							<xsl:copy-of select="page/body/password/minimum_unicode/node()"/>
						</div>

						<!-- minimum variation widget -->
						<div class="editor-block">
							<label for="minimum_variation" class="editor-title">Minimum Variation in Set of Characters:</label>
							<xsl:copy-of select="page/body/password/minimum_variation/node()"/>
						</div>

						<!-- minimum characters per set in a variation widget -->
						<div class="editor-block">
							<label for="minimum_length_of_variations" class="editor-title">Minimum Length of Each Variation:</label>
							<xsl:copy-of select="page/body/password/minimum_length_of_variations/node()"/>
						</div>

						<!-- delay between password changes widget -->
						<div class="editor-block">
							<label for="delay_between_password_changes" class="editor-title">Delay Between Password Changes (in minutes):</label>
							<xsl:copy-of select="page/body/password/delay_between_password_changes/node()"/>
						</div>

					</fieldset>

					<fieldset class="blacklist">
						<legend>Blacklist</legend>

						<!-- check password against blacklist widget -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/password/check_blacklist/node()"/>
						</div>

						<div class="editor-block">
							<label>Refuse passwords that include the username (case insensitive)</label>
							<xsl:copy-of select="page/body/password/check_username/node()"/>
						</div>
					</fieldset>

					<fieldset class="password-auto-expiration">
						<legend>Password Auto-Expiration</legend>

						<!-- password can expire? -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/password/limit_duration/node()"/>
						</div>

						<!-- how long before the password expires? -->
						<div class="editor-block">
							<label for="maximum_duration" class="editor-title">Number of days before a password expires:</label>
							<xsl:copy-of select="page/body/password/maximum_duration/node()"/>
						</div>

					</fieldset>

					<fieldset class="invalid-passwords-actions">
						<legend>Invalid Passwords Actions</legend>

						<!-- number of password failures before a block -->
						<div class="editor-block">
							<label for="invalid_passwords_counter" class="editor-title">Invalid Passwords Counter:</label>
							<xsl:copy-of select="page/body/password/invalid_passwords_counter/node()"/>
						</div>

						<!-- how long we keep the invalid password counter -->
						<div class="editor-block">
							<label for="invalid_passwords_counter_lifetime" class="editor-title">Lifetime of Invalid Passwords Counter (in hours):</label>
							<xsl:copy-of select="page/body/password/invalid_passwords_counter_lifetime/node()"/>
						</div>

						<!-- duration of a block once too may attempts were made -->
						<div class="editor-block">
							<label for="invalid_passwords_block_duration" class="editor-title">Invalid Passwords Block Duration (in hours):</label>
							<xsl:copy-of select="page/body/password/invalid_passwords_block_duration/node()"/>
						</div>

						<!-- invalid password pause before returning anything to the client -->
						<div class="editor-block">
							<label for="invalid_passwords_slowdown" class="editor-title">Invalid Passwords Slowdown (attempts multiplier):</label>
							<xsl:copy-of select="page/body/password/invalid_passwords_slowdown/node()"/>
						</div>

						<!-- number of password failures while user is blocked and before an IP block -->
						<div class="editor-block">
							<label for="blocked_user_counter" class="editor-title">User Blocked Counter:</label>
							<xsl:copy-of select="page/body/password/blocked_user_counter/node()"/>
						</div>

						<!-- how long we keep the user blocked counter -->
						<div class="editor-block">
							<label for="user_blocked_counter_lifetime" class="editor-title">Lifetime of User Blocked Counter (in days):</label>
							<xsl:copy-of select="page/body/password/blocked_user_counter_lifetime/node()"/>
						</div>

						<!-- duration of an IP block once too may attempts were made -->
						<div class="editor-block">
							<label for="blocked_user_firewall_duration" class="editor-title">User Blocked Firewall Duration:</label>
							<xsl:copy-of select="page/body/password/blocked_user_firewall_duration/node()"/>
						</div>

					</fieldset>

					<fieldset class="old-passwords-management">
						<legend>Old Passwords Management</legend>

						<!-- keep old passwords to prevent reuse? -->
						<div class="editor-block">
							<xsl:copy-of select="page/body/password/prevent_old_passwords/node()"/>
						</div>

						<!-- how many old passwords to keep around? -->
						<div class="editor-block">
							<label for="maximum_duration" class="editor-title">Minimum number of old passwords to keep in the database:</label>
							<xsl:copy-of select="page/body/password/minimum_old_passwords/node()"/>
						</div>

						<!-- how old can an old pasword become? -->
						<div class="editor-block">
							<label for="maximum_duration" class="editor-title">How old must an old password get before it gets deleted? (in days):</label>
							<xsl:copy-of select="page/body/password/old_passwords_maximum_age/node()"/>
						</div>

					</fieldset>

					<fieldset class="check-passwords">
						<legend>Check Passwords</legend>

						<div class="editor-block">
							<p>
								TODO: add a widget so one can check a sample password
								against this policy.
							</p>
						</div>
					</fieldset>

					<!--div class="buttons">
						Add support for a delete in the parent instead?
						<a class="editor-save-button" href="#">Save Changes</a>
						<a class="editor-cancel-button center-aligned" href="{/snap/head/metadata/desc[@type='page_uri']/data}">Reset</a>
						<a class="editor-delete-button right-aligned" href="#">Delete</a>
					</div-->
				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
