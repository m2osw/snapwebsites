<?xml version="1.0" encoding="UTF-8"?>
<!--
Snap Websites Server == XSLT to transform our XML Watchdog data files to HTML
Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved

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
<xsl:stylesheet version="2.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:xs="http://www.w3.org/2001/XMLSchema"
                xmlns:fn="http://www.w3.org/2005/xpath-functions"
                xmlns:html="http://www.w3.org/TR/REC-html40">

  <xsl:output method="html" encoding="utf-8" indent="yes"/>

  <xsl:template match="/">
    <html>
      <head>
        <title>Snap! Watchdog Data</title>
      </head>
      <body>
        <!--
        some of the styles are here to actually cancel the defaults
        from the snapmanagercgi.css file
        -->
        <style>
          .watchdog-data
          {
            background-color: white;
            border: 1px solid #444;
            margin: 0 20px 10px 20px;
            padding: 10px;
          }
          #error-section h2
          {
            margin-top: 0;
          }
          a
          {
            text-decoration: none;
          }
          table
          {
            border: none;
          }
          table th,
          table td
          {
            background-color: transparent;
            border: none;
          }
          table.name-value
          {
            border-spacing: 0;
            border-collapse: separate;
          }
          table.name-value th
          {
            text-align: right;
          }
          table.name-value th,
          table.name-value td
          {
            border-top: 1px solid black;
            margin: 0;
            padding: 5px;
          }
          table.name-value tr:first-child th,
          table.name-value tr:first-child td
          {
            border-top: none;
          }
          table.table-with-borders
          {
            border-left: 1px solid black;
            border-top: 1px solid black;
            border-spacing: 0;
            border-collapse: separate;
          }
          table.table-with-borders th
          {
            background-color: #e0d8e8;
          }
          table.table-with-borders th,
          table.table-with-borders td
          {
            border-right: 1px solid black;
            border-bottom: 1px solid black;
            margin: 0;
            padding: 5px;
          }
          table td.align-right
          {
            text-align: right;
          }
          table td.align-center
          {
            text-align: center;
          }
        </style>
        <h1>Snap! Watchdog Data (<xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + watchdog/@date div 1000 * xs:dayTimeDuration('PT0.001S')"/>)</h1>

        <div class="watchdog-data">

          <!-- handle the errors first so it's more prominent -->
          <xsl:variable name="error_count" select="count(watchdog/error/message)"/>
          <xsl:if test="$error_count > 0">
            <div id="error-section" style="border: 1px solid #880000; background-color: #fff0f0; padding: 10px;">
              <h2 style="color: red;">Found <xsl:value-of select="$error_count"/> Error<xsl:if test="$error_count != 1">s</xsl:if></h2>
              <table id="error-table" class="table-with-borders">
                <thead>
                  <tr>
                    <th>Plugin</th>
                    <th>Priority</th>
                    <th>Error Message</th>
                  </tr>
                </thead>
                <tbody>
                  <xsl:for-each select="watchdog/error/message">
                    <tr>
                      <td><a href="#{@plugin_name}"><xsl:value-of select="@plugin_name"/></a></td>
                      <td class="align-right"><xsl:value-of select="@priority"/></td>
                      <td><xsl:copy-of select="node()"/></td>
                    </tr>
                  </xsl:for-each>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- APT info -->
          <xsl:variable name="apt_count" select="count(watchdog/apt)"/>
          <xsl:if test="$apt_count > 0">
            <div id="apt-section">
              <h2 id="apt">Package Updates Current Status</h2>
              <table class="name-value">
                <tr>
                  <th>Time of Last Check:</th>
                  <td><xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + watchdog/apt/@last-updated div 1 * xs:dayTimeDuration('PT1S')"/></td>
                </tr>
                <tr>
                  <th>Status:</th>
                  <td>
                    <xsl:choose>
                      <xsl:when test="watchdog/apt/@total-updates and watchdog/apt/@security-updates">
                        <xsl:value-of select="@totals-updates"/> updates are available
                        <br/>
                        and
                        <br/>
                        <xsl:value-of select="watchdog/apt/@security-updates"/> security updates are available
                      </xsl:when>
                      <xsl:when test="watchdog/apt/@total-updates">
                        <xsl:value-of select="@totals-updates"/> updates are available
                      </xsl:when>
                      <!--xsl:when test="watchdog/apt/@security-updates" either you have both or only a total so this case is not necessary -->
                      <xsl:when test="watchdog/apt/@error">
                        Unknown status, errors occurred while checking for
                        updates. You may need to check this system for
                        additional information about this.
                      </xsl:when>
                      <xsl:otherwise>
                        System is up to date.
                      </xsl:otherwise>
                    </xsl:choose>
                  </td>
                </tr>
                <xsl:if test="watchdog/apt/@error">
                  <tr>
                    <th>Error:</th>
                    <td><xsl:value-of select="watchdog/apt/@error"/></td>
                  </tr>
                </xsl:if>
              </table>
            </div>
          </xsl:if>

          <!-- CPU info -->
          <xsl:variable name="cpu_count" select="count(watchdog/cpu)"/>
          <xsl:if test="$cpu_count > 0">
            <div id="cpu-section">
              <h2 id="cpu">CPU</h2>
<p>(TODO convert to readable sizes)</p>
              <table class="name-value">
                <tr>
                  <th>Time of Boot:</th>
                  <td><xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + watchdog/cpu/@time_of_boot div 1 * xs:dayTimeDuration('PT1S')"/></td>
                </tr>
                <tr>
                  <th>Up Time:</th>
                  <td><xsl:value-of select="watchdog/cpu/@uptime"/></td>
                </tr>
                <tr>
                  <th>CPU Count:</th>
                  <td><xsl:value-of select="watchdog/cpu/@cpu_count"/></td>
                </tr>
                <tr>
                  <th>CPU Freq:</th>
                  <td><xsl:value-of select="watchdog/cpu/@cpu_freq"/></td>
                </tr>
                <tr>
                  <th>Idle:</th>
                  <td><xsl:value-of select="watchdog/cpu/@idle"/></td>
                </tr>
                <tr>
                  <th>Total CPU System:</th>
                  <td><xsl:value-of select="watchdog/cpu/@total_cpu_system"/></td>
                </tr>
                <tr>
                  <th>Total CPU User:</th>
                  <td><xsl:value-of select="watchdog/cpu/@total_cpu_user"/></td>
                </tr>
                <tr>
                  <th>Total CPU Wait:</th>
                  <td><xsl:value-of select="watchdog/cpu/@total_cpu_wait"/></td>
                </tr>
                <tr>
                  <th>Avg 1 min.:</th>
                  <td><xsl:value-of select="watchdog/cpu/@avg1"/></td>
                </tr>
                <tr>
                  <th>Avg 5 min.:</th>
                  <td><xsl:value-of select="watchdog/cpu/@avg5"/></td>
                </tr>
                <tr>
                  <th>Avg 15 min.:</th>
                  <td><xsl:value-of select="watchdog/cpu/@avg15"/></td>
                </tr>
                <tr>
                  <th>Page Cache In:</th>
                  <td><xsl:value-of select="watchdog/cpu/@page_cache_in"/></td>
                </tr>
                <tr>
                  <th>Page Cache Out:</th>
                  <td><xsl:value-of select="watchdog/cpu/@page_cache_out"/></td>
                </tr>
                <tr>
                  <th>Swap Cache In:</th>
                  <td><xsl:value-of select="watchdog/cpu/@swap_cache_in"/></td>
                </tr>
                <tr>
                  <th>Swap Cache Out:</th>
                  <td><xsl:value-of select="watchdog/cpu/@swap_cache_out"/></td>
                </tr>
                <tr>
                  <th>Total Processes:</th>
                  <td><xsl:value-of select="watchdog/cpu/@total_processes"/></td>
                </tr>
              </table>
            </div>
          </xsl:if>

          <!-- Memory info -->
          <xsl:variable name="memory_count" select="count(watchdog/memory)"/>
          <xsl:if test="$memory_count > 0">
            <div id="memory-section">
              <h2 id="memory">Memory</h2>
<p>(TODO convert to readable sizes)</p>
              <table class="name-value">
                <tr>
                  <th>Total Memory:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_total"/></td>
                </tr>
                <tr>
                  <th>Available Memory:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_available"/></td>
                </tr>
                <tr>
                  <th>Free Memory:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_free"/></td>
                </tr>
                <tr>
                  <th>Memory Buffers:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_buffers"/></td>
                </tr>
                <tr>
                  <th>Memory Cached:</th>
                  <td><xsl:value-of select="watchdog/memory/@mem_cached"/></td>
                </tr>
                <tr>
                  <th>Swap Total Space:</th>
                  <td><xsl:value-of select="watchdog/memory/@swap_total"/></td>
                </tr>
                <tr>
                  <th>Free Swap Space:</th>
                  <td><xsl:value-of select="watchdog/memory/@swap_free"/></td>
                </tr>
                <tr>
                  <th>Swap Space Cached:</th>
                  <td><xsl:value-of select="watchdog/memory/@swap_cached"/></td>
                </tr>
              </table>
            </div>
          </xsl:if>

          <!-- Disk info -->
          <xsl:variable name="disk_count" select="count(watchdog/disk)"/>
          <xsl:if test="$disk_count > 0">
            <div id="disk-section">
              <h2 id="disk">Disk Partitions</h2>
<p>(TODO convert to readable sizes)</p>
              <table class="table-with-borders">
                <thead>
                  <tr>
                    <th>Path</th>
                    <th>Free Space</th>
                    <th>Block Free</th>
                    <th>Blocks</th>
                    <th>Flags</th>
                    <th>Free Files</th>
                    <th>Files Available</th>
                    <th>Error</th>
                  </tr>
                </thead>
                <tbody>
                  <xsl:for-each select="watchdog/disk/partition">
                    <tr>
                      <td><xsl:value-of select="@dir"/></td>
                      <td class="align-right"><xsl:value-of select="@available"/></td>
                      <td class="align-right"><xsl:value-of select="@bfree"/></td>
                      <td class="align-right"><xsl:value-of select="@blocks"/></td>
                      <td class="align-right"><xsl:value-of select="@flags"/></td>
                      <td class="align-right"><xsl:value-of select="@ffree"/></td>
                      <td class="align-right"><xsl:value-of select="@favailable"/></td>
                      <td><xsl:value-of select="@error"/></td>
                    </tr>
                  </xsl:for-each>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- one log entry per log file probed -->
          <xsl:variable name="logs_count" select="count(watchdog/logs)"/>
          <xsl:if test="$logs_count > 0">
            <div id="logs-section">
              <h2 id="log">Logs</h2>
<p>(TODO convert the mode and make size human readable--with byte size in a title=... and uid/gid should be in text too)</p>
              <table class="table-with-borders">
                <thead>
                  <tr>
                    <th>Group</th>
                    <th>Filename</th>
                    <th>Size</th>
                    <th>User ID</th>
                    <th>Group ID</th>
                    <th>Mode</th>
                    <th>Last Modified Date</th>
                    <th>Error</th>
                  </tr>
                </thead>
                <tbody>
                  <!-- TODO: look in a way to get group names shown once -->
                  <xsl:for-each select="watchdog/logs/log">
                    <tr>
                      <td><xsl:value-of select="@name"/></td>
                      <td><xsl:value-of select="@filename"/></td>
                      <td class="align-right"><xsl:value-of select="@size"/></td>
                      <td class="align-right"><xsl:value-of select="@uid"/></td>
                      <td class="align-right"><xsl:value-of select="@gid"/></td>
                      <td class="align-right"><xsl:value-of select="@mode"/></td>
                      <td class="align-right"><xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + @mtime div 1 * xs:dayTimeDuration('PT1S')"/></td>
                      <td><xsl:value-of select="@error"/></td>
                    </tr>
                  </xsl:for-each>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- one list entry per list file probed -->
          <xsl:variable name="journals_count" select="count(watchdog/list/journal)"/>
          <xsl:if test="$journals_count > 0">
            <div id="lists-section">
              <h2 id="list">Lists</h2>
<p>(TODO convert the mode and make size human readable--with byte size in a title=... and uid/gid should be in text too)</p>
              <table class="table-with-borders">
                <thead>
                  <tr>
                    <th>Filename</th>
                    <th>Size</th>
                    <th>User ID</th>
                    <th>Group ID</th>
                    <th>Mode</th>
                    <th>Last Modified Date</th>
                    <th>Error</th>
                  </tr>
                </thead>
                <tbody>
                  <!-- TODO: look in a way to get group names shown once -->
                  <xsl:for-each select="watchdog/list/journal">
                    <tr>
                      <td><xsl:value-of select="@filename"/></td>
                      <td class="align-right"><xsl:value-of select="@size"/></td>
                      <td class="align-right"><xsl:value-of select="@uid"/></td>
                      <td class="align-right"><xsl:value-of select="@gid"/></td>
                      <td class="align-right"><xsl:value-of select="@mode"/></td>
                      <td class="align-right"><xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + @mtime div 1 * xs:dayTimeDuration('PT1S')"/></td>
                      <td><xsl:value-of select="@error"/></td>
                    </tr>
                  </xsl:for-each>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- flags were raised -->
          <xsl:variable name="flags_count" select="count(watchdog/flags/flag)"/>
          <xsl:if test="$flags_count > 0">
            <div id="flags-section">
              <h2 id="flags">Flags</h2>
              <p>
                <strong>WARNING:</strong> all flags are errors that the administrator
                needs to look into and correct until they all disappear. Remember
                that for some flags you may have to restart a service before it gets
                updated (see the <em>Modified</em> column.) In the worst case
                scenario, you may have to reboot to see that your changes had the
                expected effect. Flags that are marked as manual ("yes" in the
                manual column) will never be taken down automatically. For those
                click on the <strong>Take Down</strong> link or run the raise-flag
                command as shown (hover your mouse over the Manual column to see
                the command in the tooltip.)
              </p>
              <table class="table-with-borders">
                <thead>
                  <th>Unit</th>
                  <th>Section</th>
                  <th>Name</th>
                  <th>Source File</th>
                  <th>Function</th>
                  <th>Line</th>
                  <th>Message</th>
                  <th>Priority</th>
                  <th>Manual</th>
                  <th>Date</th>
                  <th>Modified</th>
                  <th>Tags</th>
                  <th>Action</th>
                </thead>
                <tbody>
                  <xsl:for-each select="watchdog/flags/flag">
                    <tr>
                      <td><xsl:value-of select="@unit"/></td>
                      <td><xsl:value-of select="@section"/></td>
                      <td><xsl:value-of select="@name"/></td>
                      <td><xsl:value-of select="source/@source-file"/></td>
                      <td><xsl:value-of select="source/@function"/></td>
                      <td class="align-right"><xsl:value-of select="source/@line"/></td>
                      <td><xsl:value-of select="message/node()"/></td>
                      <td class="align-right"><xsl:value-of select="@priority"/></td>
                      <td class="align-center">
                        <xsl:if test="manual-down = 'yes'">
                          <xsl:attribute name="title">
                            Use command line `raise-flag --down
                            <xsl:value-of select="@unit"/>
                            <xsl:value-of select="@section"/>
                            <xsl:value-of select="@name"/>`
                            to take this flag down so it doesn't appear here
                            anymore (unless the problem was not resolved)
                          </xsl:attribute>
                        </xsl:if>
                        <xsl:value-of select="manual-down/node()"/>
                      </td>
                      <td><xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + date div 1 * xs:dayTimeDuration('PT1S')"/></td>
                      <td><xsl:value-of select="xs:dateTime('1970-01-01T00:00:00') + modified div 1 * xs:dayTimeDuration('PT1S')"/></td>
                      <td><xsl:copy-of select="string-join(tags/tag, ', ')"/></td>
                      <td>
                        <!-- TBD: should we display the link only if manual-down="yes"? -->
                        <a>
                          <xsl:attribute name="href">/snapmanager?host=...TBD...&amp;watchdog_flag=<xsl:value-of
                            select="@unit"/>/<xsl:value-of
                            select="@section"/>/<xsl:value-of
                            select="@name"/>&amp;action=down</xsl:attribute>
                          Take Down
                        </a>
                        (Please implement, tweak link as required by existing interface [or possibly replace with a form?]...)
                      </td>
                    </tr>
                  </xsl:for-each>
                </tbody>
              </table>
              <p><xsl:value-of select="$flags_count"/> flags raised.</p>
            </div>
          </xsl:if>

          <!-- cassandra has a process when running -->
          <xsl:variable name="cassandra_count" select="count(watchdog/cassandra)"/>
          <xsl:if test="$cassandra_count > 0">
            <div id="cassandra-section">
              <h2 id="cassandra">Cassandra</h2>
<p>(TODO this should be part of a file in snapdbproxy)</p>
              <table class="table-with-borders">
                <xsl:call-template name="process_table_header"/>
                <tbody>
                  <xsl:apply-templates select="watchdog/cassandra/process"/>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- firewall has a process when running -->
          <xsl:variable name="firewall_count" select="count(watchdog/firewall)"/>
          <xsl:if test="$firewall_count > 0">
            <div id="firewall-section">
              <h2 id="firewall">Firewall</h2>
<p>(TODO this should be part of a file in snapfirewall)</p>
              <table class="table-with-borders">
                <xsl:call-template name="process_table_header"/>
                <tbody>
                  <xsl:apply-templates select="watchdog/firewall/process"/>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- network has a process when running -->
          <xsl:variable name="network_count" select="count(watchdog/network)"/>
          <xsl:if test="$network_count > 0">
            <div id="network-section">
              <h2 id="network">Network</h2>
<p>(TODO this should be part of a file in snapcommunicator)</p>
              <table class="table-with-borders">
                <xsl:call-template name="process_table_header"/>
                <tbody>
                  <xsl:apply-templates select="watchdog/network/process"/>
                </tbody>
              </table>
            </div>
          </xsl:if>

          <!-- packages has a process when running -->
          <xsl:variable name="package_count" select="count(watchdog/packages/package)"/>
          <xsl:if test="$package_count > 0">
            <div id="packages-section">
              <h2 id="packages">Packages</h2>
              <p>
                Packages listed here are either required, unwanted, or may be
                in conflict with other packages. This table shows one of these
                errors for each one of the packages listed. If no error is
                shown then the package is conforming to the expectation of
                snapwatchdog and there is nothing to worry about. If a
                package is shown as missing, search for it in the bundles
                and install the corresponding bundle. If the package is
                shown as unwanted, you've got to have been installing that
                one by hand and it will require you to purge it manually.
                A package marked as in conflict with another (or several
                others)  should be reviewed closely to know whether it
                causes problems on your systme or not. If not, you may
                want to leave it alone. Otherwise, think about removing
                all the packages generating a conflict.
              </p>
              <table class="table-with-borders">
                <thead>
                  <th>Name</th>
                  <th>Installation</th>
                  <th>Conflicts</th>
                  <th>Errors</th>
                </thead>
                <tbody>
                  <xsl:for-each select="watchdog/packages/package">
                    <tr>
                      <td><xsl:value-of select="@name"/></td>
                      <td><xsl:value-of select="@installation"/></td>
                      <td><xsl:value-of select="@conflicts"/></td>
                      <td><xsl:value-of select="@error"/></td>
                    </tr>
                  </xsl:for-each>
                </tbody>
              </table>
              <xsl:value-of select="$package_count"/> package<xsl:if test="$package_count != 1">s</xsl:if>
            </div>
          </xsl:if>

          <!-- processes has a process when running -->
          <xsl:variable name="processes_count" select="count(watchdog/processes/process)"/>
          <xsl:if test="$processes_count > 0">
            <div id="processes-section">
              <h2 id="processes">Processes</h2>
              <table class="table-with-borders">
                <xsl:call-template name="process_table_header"/>
                <tbody>
                  <xsl:apply-templates select="watchdog/processes/process"/>
                </tbody>
              </table>
              <xsl:value-of select="$processes_count"/> process<xsl:if test="$processes_count != 1">es</xsl:if>
            </div>
          </xsl:if>

          <!-- watchscripts has a set of results, one per script -->
          <xsl:variable name="watchscripts_count" select="count(watchdog/watchscripts/script)"/>
          <xsl:if test="$watchscripts_count > 0">
            <div id="watchscripts-section">
              <h2 id="watchscripts">Watchscripts</h2>
              <table class="table-with-borders">
                <thead>
                  <tr>
                    <th>Script Path</th>
                    <th>Exit Code</th>
                  </tr>
                </thead>
                <tbody>
                  <xsl:for-each select="watchdog/watchscripts/script">
                    <tr>
                      <td>
                        <xsl:value-of select="@name"/>
                        <xsl:if test="output or error">
                          <br/>
                          <br/>
                          <xsl:for-each select="tokenize(output, '\n')">
                            <xsl:copy-of select="."/>
                            <br/>
                          </xsl:for-each>
                          <xsl:for-each select="tokenize(error, '\n')">
                            <xsl:copy-of select="."/>
                            <br/>
                          </xsl:for-each>
                        </xsl:if>
                      </td>
                      <td class="align-right"><xsl:value-of select="@exit_code"/></td>
                    </tr>
                  </xsl:for-each>
                </tbody>
              </table>
            </div>
          </xsl:if>

        </div>
      </body>
    </html>
  </xsl:template>

  <xsl:template name="process_table_header">
    <thead>
      <tr>
        <th>Name</th>
        <th>Resident</th>
        <th>PCPU</th>
        <th>UTime</th>
        <th>CUTime</th>
        <th>STime</th>
        <th>CSTime</th>
        <th>TSize</th>
        <th>TTY</th>
        <th>Error</th>
      </tr>
    </thead>
  </xsl:template>

  <xsl:template match="process">
    <tr>
      <xsl:attribute name="class">
        process
        <xsl:if test="@error">
          error
        </xsl:if>
        <xsl:if test="@tty != 0">
          tty
        </xsl:if>
      </xsl:attribute>
      <td><xsl:value-of select="@name"/></td>
      <td class="align-right"><xsl:value-of select="@resident"/></td>
      <td class="align-right"><xsl:value-of select="@pcpu"/></td>
      <td class="align-right"><xsl:value-of select="@utime"/></td>
      <td class="align-right"><xsl:value-of select="@utime"/></td>
      <td class="align-right"><xsl:value-of select="@stime"/></td>
      <td class="align-right"><xsl:value-of select="@cstime"/></td>
      <td class="align-right"><xsl:value-of select="@total_size"/></td>
      <td class="align-center"><xsl:if test="@tty != 0">Yes</xsl:if></td>
      <td><xsl:value-of select="@error"/></td>
    </tr>
  </xsl:template>

</xsl:stylesheet>
<!--
vim: ts=2 sw=2 et
-->
