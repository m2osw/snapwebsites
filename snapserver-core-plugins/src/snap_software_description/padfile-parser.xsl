<?xml version="1.0"?>
<!--
Snap Websites Server == PADFile data to XML
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
	<!-- include system data -->
	<xsl:include href="functions"/>

	<!-- some special variables to define the theme -->
	<xsl:variable name="layout-name">padfile</xsl:variable>
	<xsl:variable name="layout-area">padfile-parser</xsl:variable>
	<xsl:variable name="layout-modified">2014-11-29 02:35:36</xsl:variable>

	<xsl:template name="alternate-format">
		<xsl:param name="href"/>
		<xsl:param name="type"/>
		<xsl:param name="title"/>
		<xsl:if test="$type = 'application/rss+xml'">
			<!-- Could we also add our Atom? -->
			<RSS>
				<RSS_FORM>Y</RSS_FORM>
				<RSS_DESCRIPTION>RSS Feed Extension</RSS_DESCRIPTION>
				<RSS_VERSION>1.0</RSS_VERSION>
				<RSS_URL>http://pad.asp-software.org/extensions/RSS.htm</RSS_URL>
				<RSS_SCOPE>Product</RSS_SCOPE>
				<RSS_Feed_Company><xsl:value-of select="$href"/></RSS_Feed_Company>
				<RSS_Feed_Product><xsl:value-of select="$href"/></RSS_Feed_Product>
			</RSS>
		</xsl:if>
	</xsl:template>

	<xsl:template match="snap">
		<output>
			<XML_DIZ_INFO>
				<MASTER_PAD_VERSION_INFO>
					<MASTER_PAD_VERSION>3.11</MASTER_PAD_VERSION>
					<MASTER_PAD_EDITOR>Snap Websites v<xsl:value-of select="head/metadata/desc[@type='version']/data"/></MASTER_PAD_EDITOR>
					<MASTER_PAD_INFO>http://www.asp-shareware.org/pad/spec/spec.php</MASTER_PAD_INFO>
				</MASTER_PAD_VERSION_INFO>
				<Company_Info>
					<Company_Name></Company_Name>
					<Address1></Address1>
					<Address2></Address2>
					<City_Town></City_Town>
					<State_Province></State_Province>
					<Zip_Postal_Code></Zip_Postal_Code>
					<Country></Country>
					<Company_WebSite_URL></Company_WebSite_URL>
					<Contact_Info>
						<Author_First_Name></Author_First_Name>
						<Author_Last_Name></Author_Last_Name>
						<Author_Email></Author_Email>
						<Contact_First_Name></Contact_First_Name>
						<Contact_Last_Name></Contact_Last_Name>
						<Contact_Last_Email></Contact_Last_Email>
					</Contact_Info>
					<Support_Info>
						<Sales_Email></Sales_Email>
						<Support_Email></Support_Email>
						<General_Email></General_Email>
						<Sales_Phone></Sales_Phone>
						<Support_Phone></Support_Phone>
						<General_Phone></General_Phone>
						<Fax_Phone></Fax_Phone>
					</Support_Info>
				</Company_Info>
				<Program_Info>
					<Program_Name></Program_Name>
					<Program_Version></Program_Version>
					<Program_Release_Month></Program_Release_Month>
					<Program_Release_Day></Program_Release_Day>
					<Program_Release_Year></Program_Release_Year>
					<Program_Cost_Dollars></Program_Cost_Dollars>
					<Program_Cost_Other_Code></Program_Cost_Other_Code>
					<Program_Cost_Other></Program_Cost_Other>
					<Program_Type></Program_Type>
					<Program_Release_Status></Program_Release_Status>
					<Program_Install_Support></Program_Install_Support>
					<Program_OS_Support></Program_OS_Support>
					<Program_Language></Program_Language>
					<File_Info>
						<File_Size_Bytes></File_Size_Bytes>
						<File_Size_K></File_Size_K>
						<File_Size_MB></File_Size_MB>
					</File_Info>
					<Expire_Info>
						<Has_Expire_Info></Has_Expire_Info>
						<Expire_Count></Expire_Count>
						<Expire_Based_On></Expire_Based_On>
						<Expire_Other_Info></Expire_Other_Info>
						<Expire_Month></Expire_Month>
						<Expire_Day></Expire_Day>
						<Expire_Year></Expire_Year>
					</Expire_Info>
					<Program_Change_Info></Program_Change_Info>
					<Program_Category_Class></Program_Category_Class>
					<Program_System_Requirements></Program_System_Requirements>
				</Program_Info>
				<Program_Descriptions>
					<language-name>
						<Keywords></Keywords>
						<Char_Desc_45></Char_Desc_45>
						<Char_Desc_80></Char_Desc_80>
						<Char_Desc_250></Char_Desc_250>
						<Char_Desc_450></Char_Desc_450>
						<Char_Desc_2000></Char_Desc_2000>
					</language-name>
				</Program_Descriptions>
				<Web_Info>
					<Application_URLs>
						<Application_Info_URL></Application_Info_URL>
						<Application_Order_URL></Application_Order_URL>
						<Application_Screenshot_URL></Application_Screenshot_URL>
						<Application_Icon_URL></Application_Icon_URL>
						<Application_XML_File_URL></Application_XML_File_URL>
					</Application_URLs>
					<Download_URLs>
						<Primary_Download_URL></Primary_Download_URL>
						<Secondary_Download_URL></Secondary_Download_URL>
						<Secondary_Download_URL_1></Secondary_Download_URL_1>
						<Secondary_Download_URL_2></Secondary_Download_URL_2>
					</Download_URLs>
				</Web_Info>
				<Permissions>
					<Distribution_Permissions></Distribution_Permissions>
					<EULA></EULA>
				</Permissions>

				<!-- add a reference to our PADmap file so a spider can find all PADfiles at once -->
				<PADmap>
					<PADmap_FORM>Y</PADmap_FORM>
					<PADmap_DESCRIPTION>Link to plain text file containing all your PAD URLs from current host</PADmap_DESCRIPTION>
					<PADmap_VERSION>1.0</PADmap_VERSION>
					<PADmap_URL>http://padmaps.org/protocol</PADmap_URL>
					<PADmap_SCOPE>Company</PADmap_SCOPE>
					<PADmap_Location><xsl:value-of select="$website_uri"/>padmap.txt</PADmap_Location>
				</PADmap>

				<!-- if available, add RSS feed URLs. -->
				<xsl:for-each select="page/body/formats">
					<xsl:call-template name="alternate-format">
						<xsl:with-param name="href" select="@href"/>
						<xsl:with-param name="type" select="@type"/>
						<xsl:with-param name="title" select="."/>
					</xsl:call-template>
				</xsl:for-each>

				<!--Affiliates> == I'm not too sure how to support that extension... because some names include a variable ($c).
					<Affiliates_FORM>Y</Affiliates_FORM>
					<Affiliates_DESCRIPTION>Affiliates Extension</Affiliates_DESCRIPTION>
					<Affiliates_VERSION>1.4</Affiliates_VERSION>
					<Affiliates_URL>http://pad.asp-software.org/extensions/Affiliates.htm</Affiliates_URL>
					<Affiliates_SCOPE>Product</Affiliates_SCOPE>', "\n";
					<Affiliates_Information_Page></Affiliates_Information_Page>
					<Affiliates_ $c _Order_Page></Affiliates_ $c _Order_Page>
					<Affiliates_ $c _Vendor_ID></Affiliates_ $c _Vendor_ID>
					<Affiliates_ $c _Product_ID></Affiliates_ $c _Product_ID>
					<Affiliates_ $c _Maximum_Commission_Rate></Affiliates_ $c _Maximum_Commission_Rate>\n";
				</Affiliates-->

			</XML_DIZ_INFO>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
