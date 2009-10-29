<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times"
             xmlns:I18N="http://www.gnu.org/software/gettext/" extension-element-prefixes="I18N">

<!--
  Copyright (C) 2004-2005 Imendio AB
  Copyright (c) 2003 Daniel Lundin
  Copyright (c) 2003 CodeFactory AB
  Copyright (c) 2004 Chris Ladd (caladd@particlestorm.net)
-->

<!-- ********************************************************************* -->
<!--                            Output settings                            -->

<xsl:output 
  method="xml"
  encoding="utf-8"
  indent="yes"
  doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN"
  doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"
/>


<!-- ********************************************************************* -->
<!--                           Global variables                            -->

<!-- Indentation-level in pixels for hierarchical tasks -->
<xsl:variable name="task-indent-pixels" select="18"/>

<!-- CSS file to be included in XHTML output -->
<xsl:variable name="css-stylesheet-local" select="'html1_css.xsl'"/>
<xsl:variable name="css-stylesheet-local-ie" select="'html1_css_ie.xsl'"/>
<xsl:variable name="css-stylesheet-local-ie7" select="'html1_css_ie7.xsl'"/>


<!-- Current date/time at UTC/GMT -->
<xsl:variable name="datetime-utc">
  <xsl:variable name="lt" select="date:date-time ()"/>
  <xsl:variable name="secs" select="date:seconds ()"/>
  <xsl:variable name="tz">
    <xsl:choose>
      <!-- UTC -->
      <xsl:when test="substring ($lt, 20, 1) = 'Z'">
        0
      </xsl:when>
      <!-- East of UTC -->
      <xsl:when test="substring ($lt, 20, 1) = '+'">
        <xsl:value-of select="concat('-', (3600*substring ($lt, 21, 2))+
                              (60*substring ($lt, 24,2)))"/>
      </xsl:when>
      <!-- West of UTC -->
      <xsl:otherwise>
        <xsl:value-of select="(3600*substring ($lt, 21, 2))+
                              (60*substring ($lt, 24,2))"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:value-of select="date:add ($lt, date:duration (2 * $tz))"/>
</xsl:variable>

<xsl:variable name="projstart">
  <xsl:call-template name="mrproj-parse-date">
    <xsl:with-param name="thedate" select="//project/@project-start"/>
  </xsl:call-template>
</xsl:variable>

<!-- Determine project length in seconds -->
<xsl:variable name="projlength">
  <xsl:choose>
    <xsl:when test="//project//task">
      <xsl:for-each select="//project//task">
        <xsl:sort data-type="number"
         select="date:seconds(date:add (date:date (concat (substring (@end, 1, 4),
                                                       '-', substring (@end, 5, 2),
                                                       '-', substring (@end, 7, 2))),
                                   date:duration ((3600 * substring (@end, 10, 2))+
                                                  (60 * substring (@end, 12, 2))+
                                                  substring (@end, 14, 2))))"
          order="descending"/>
        <xsl:if test="position()=1">
          <xsl:copy-of
            select="-(date:seconds ($projstart) - date:seconds (
                               date:add (date:date (concat (substring (@end, 1, 4),
                                                     '-', substring (@end, 5, 2),
                                                     '-', substring (@end, 7, 2))),
                                   date:duration ((3600 * substring (@end, 10, 2))+
                                                  (60 * substring (@end, 12, 2))+
                                                  substring (@end, 14, 2)))))"/>
          </xsl:if>
        </xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        0
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

<xsl:variable name="projend" select="date:add($projstart, date:duration($projlength))"/>

<!-- ********************************************************************* -->
<!--                     Include the actual templates                      -->

<xsl:include href="html1_tasks.xsl"/>
<xsl:include href="html1_resources.xsl"/>
<xsl:include href="html1_gantt.xsl"/>


<!-- ********************************************************************* -->
<!--                      Common templates/functions                       -->

<!-- Parse a date into an EXSLT date:date -->
<xsl:template name="mrproj-parse-date">
  <xsl:param name="thedate"/>
  <xsl:variable name="formatted" select="concat(substring($thedate, 1, 4),
   '-', substring($thedate, 5, 2), '-', substring($thedate, 7, 2), 
   substring($thedate, 9, 3), ':', substring($thedate, 12, 2), ':',
   substring($thedate, 14, 3))"/>

  <xsl:value-of select="date:add($formatted, date:duration(0))"/>
</xsl:template>


<!-- Present a date:duration in human readable form -->
<xsl:template name="mrproj-duration">
  <xsl:param name="duration-in-seconds"/>
  <xsl:variable name="days" select="floor($duration-in-seconds div 28800)"/>
  <xsl:variable name="hours" select="floor(($duration-in-seconds mod 28800) div 3600)"/>
  <xsl:if test="$days != '0'">
    <xsl:value-of select="$days"/>
    <xsl:text>d </xsl:text>
  </xsl:if>
  <xsl:if test="$hours != '0'">
    <xsl:value-of select="$hours"/>
    <xsl:text>h </xsl:text>
  </xsl:if>
</xsl:template>
  
<!-- List the resources along with their assigned units if not 100 -->
<xsl:template name="mrproj-assigned-resources">
  <xsl:param name="task-id"/>

  <xsl:for-each select="/project/allocations/allocation[@task-id=$task-id]">
    <xsl:sort data-type="number" select="@resource-id" order="ascending"/>
    <xsl:variable name="resource-id" select="@resource-id"/>

    <!-- Use the short name of a resource unless it's empty -->
    <xsl:choose>
      <xsl:when test="/project/resources/resource[@id=$resource-id]/@short-name = ''">
        <xsl:value-of select="/project/resources/resource[@id=$resource-id]/@name"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="/project/resources/resource[@id=$resource-id]/@short-name"/>
      </xsl:otherwise>
    </xsl:choose>

    <!-- Create a tag displaying the number of units this resource spends on this task if it is not 100 -->
    <xsl:variable name="units" select="/project/allocations/allocation[@resource-id=$resource-id and @task-id=$task-id]/@units"/>
    <xsl:if test="$units != 100">[<xsl:value-of select="$units"/>]</xsl:if>

    <xsl:if test="not(position() = last())">
      <xsl:text>, </xsl:text>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

<!-- ********************************************************************* -->
<!--                             XHTML page header                         -->
<xsl:template name="htmlhead">
  <xsl:param name='title'/>
  <head>

      <xsl:comment>
              This file is generated from xml source: DO NOT EDIT
      </xsl:comment>

    <title><xsl:value-of select="$title"/> - Planner</title>
    <meta name="GENERATOR" content="Planner HTML output"/>
    <style type="text/css">
      <xsl:value-of select="document($css-stylesheet-local)"/>
    </style>
      <xsl:comment>
        <xsl:text>[if IE]&gt;&lt;style type="text/css"&gt;</xsl:text>
        <xsl:value-of select="document($css-stylesheet-local-ie)"/>
        <xsl:text>&lt;/style&gt;&lt;![endif]</xsl:text>
      </xsl:comment>
      <xsl:comment>
        <xsl:text>[if gte IE 7]&gt;&lt;style type="text/css"&gt;</xsl:text>
        <xsl:value-of select="document($css-stylesheet-local-ie7)"/>
        <xsl:text>&lt;/style&gt;&lt;![endif]</xsl:text>
      </xsl:comment>
  </head>
</xsl:template>

<!-- ********************************************************************* -->
<!--                                Footer                                 -->
<xsl:template name="htmlfooter">
  <div class="footer">
    <div>
      <xsl:value-of select="I18N:gettext('This file was generated by')"/>&nbsp;
      <a href="http://live.gnome.org/Planner/" style="text-decoration: underline;">Planner</a>
    </div>
  </div>
</xsl:template>

<xsl:template name="property">
    <xsl:if test="@value!=''">
      <div class="property">
        <span class="property-name"><xsl:value-of select="@name"/></span>:
        <span class="property-value"><xsl:value-of select="@value"/></span>
      </div>
    </xsl:if>
</xsl:template>

<!-- ********************************************************************* -->
<!--                             Main Template                             -->

<xsl:template match="project">
  <!-- Project start date -->
  <xsl:variable name="projstartdate">
    <xsl:call-template name="mrproj-parse-date">
      <xsl:with-param name="thedate" select="@project-start"/>
    </xsl:call-template>
  </xsl:variable>


  <html>
    <xsl:call-template name="htmlhead">
      <xsl:with-param name="title"><xsl:value-of select="@name"/></xsl:with-param>
    </xsl:call-template>

    <body>

      <h1 class="proj-title">
        <a name="project">
        <xsl:choose>
          <xsl:when test="@name != ''">
            <xsl:value-of select="@name"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="I18N:gettext('Unnamed Project')"/>
          </xsl:otherwise>
        </xsl:choose>
        </a>
      </h1>

<table class="proj-header">

<xsl:if test="@company != ''">
<tr>
  <td class="header"><xsl:value-of select="I18N:gettext('Company:')"/></td>
  <td>
    <xsl:value-of select="@company"/>
  </td>
</tr>
</xsl:if>

<xsl:if test="@manager != ''">
<tr>
  <td class="header"><xsl:value-of select="I18N:gettext('Manager:')"/></td>
  <td>
    <xsl:value-of select="@manager"/>
  </td>
</tr>
</xsl:if>

<tr>
  <td class="header"><xsl:value-of select="I18N:gettext('Start:')"/></td>
  <td>
    <xsl:value-of select="I18N:getdate(date:seconds($projstart))"/>
  </td>
</tr>

<tr>
  <td class="header"><xsl:value-of select="I18N:gettext('Finish:')"/></td>
  <td>
    <xsl:value-of select="I18N:getdate(date:seconds($projend))"/>
  </td>
</tr>

<!--tr>
  <td class="header">Duration:</td>
  <td>
    <xsl:value-of select="floor($projlength div 86400)"/>d
    <xsl:value-of select="floor($projlength div 3600) mod 24"/>h
  </td>
</tr-->

<xsl:if test="@phase != ''">
<tr>
  <td class="header"><xsl:value-of select="I18N:gettext('Phase:')"/></td>
  <td>
    <xsl:value-of select="@phase"/>
  </td>
</tr>
</xsl:if>

<xsl:for-each select="properties/property[@owner='project']">
<tr>
  <td class="header"><xsl:value-of select="@label"/><xsl:text>:</xsl:text></td>
  <td>
    <xsl:variable name="name" select="@name"/>
    <xsl:choose>
      <xsl:when test="@type='float'">
        <xsl:value-of select="format-number(/project/properties/property[@name=$name]/@value, '.####')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="/project/properties/property[@name=$name]/@value"/>
      </xsl:otherwise>
    </xsl:choose>
  </td>
</tr>
</xsl:for-each>

<tr>
  <td class="header"><xsl:value-of select="I18N:gettext('Report Date:')"/></td>
  <td>
    <xsl:value-of select="I18N:getdate(date:seconds(date:date()))"/>
  </td>
</tr>
</table>

      <div class="separator"/>

      <!-- Defined in html1_gantt.xsl -->
      <xsl:call-template name="gantt"/> 

      <div class="separator"/>
      
      <!-- Defined in html1_tasks.xsl -->
      <xsl:apply-templates select="tasks"/> 

      <div class="separator"/>

      <!-- Defined in html1_resources.xsl -->
      <xsl:apply-templates select="resources"/> 

      <xsl:call-template name="htmlfooter"/>
    </body>
  </html>
</xsl:template>

</xsl:stylesheet>
