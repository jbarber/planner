<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times">

<!--**************************************************************************
    *
    * html1.xsl: Convert a Planner XML file to HTML
    *
    * Copyright (c)2003 Daniel Lundin
    * Copyright (c)2003 CodeFactory AB
    *
    *-->

<!--/\___________________________________________________________________/\-->
<!--                            Output settings                            -->

<xsl:output 
  method="xml"
  encoding="ISO-8859-1"
  indent="yes"
  doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN"
  doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"
/>


<!--/\___________________________________________________________________/\-->
<!--                           Global variables                            -->

<!-- Indentation-level in pixels for hierarchical tasks -->
<xsl:variable name="task-indent-pixels" select="18"/>

<!-- CSS file to be included in XHTML output -->
<xsl:variable name="css-stylesheet-local" select="'html1_css.xsl'"/>


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
  </xsl:variable>

<!--/\___________________________________________________________________/\-->
<!--                     Include the actual templates                      -->

<xsl:include href="html1_tasks.xsl"/>
<xsl:include href="html1_resources.xsl"/>
<xsl:include href="html1_gantt.xsl"/>
<xsl:include href="html1_milestones.xsl"/>


<!--/\___________________________________________________________________/\-->
<!--                      Common templates/functions                       -->

<!-- Parse a date into an EXSLT date:date -->
<xsl:template name="mrproj-parse-date">
  <xsl:param name="thedate"/>
  <xsl:value-of 
            select="date:add(date:date (concat (substring ($thedate, 1, 4),
             '-', substring ($thedate, 5, 2),
             '-', substring ($thedate, 7, 2))), 
        date:duration ((3600 * substring ($thedate, 10, 2))+
                           (60 * substring ($thedate, 12, 2))+
                            substring ($thedate, 14, 2)))"/>
</xsl:template>


<!-- Present a date:duration in human readable form -->
<xsl:template name="mrproj-duration">
  <xsl:param name="duration"/>
  <xsl:variable name="days" select="substring-before ($duration, 'D')"/>
  <xsl:variable name="hours" select="substring-before (
                                       substring-after ($duration, 'T'),
                                       'H')"/>
  <xsl:if test="$days != ''">
    <xsl:value-of select="$days"/>
    <xsl:text>d </xsl:text>
  </xsl:if>
  <xsl:if test="$hours != ''">
    <xsl:value-of select="$hours"/>
    <xsl:text>h </xsl:text>
  </xsl:if>
</xsl:template>
  

<!-- XHTML page header -->
<xsl:template name="htmlhead">
  <xsl:param name='title'/>
  <head>

      <xsl:comment>
        XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

              This file is generated from xml source: DO NOT EDIT

        XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
      </xsl:comment>

    <title><xsl:value-of select="$title"/> - Planner</title>
    <meta name="GENERATOR" content="Planner HTML output"/>
    <style type="text/css">
      <xsl:value-of select="document($css-stylesheet-local)"/>
    </style>
  </head>
</xsl:template>


<!-- XHTML page footer -->
<xsl:template name="htmlfooter">
  <div class="footer">
  <div class="footer-date">
    Created 
    <xsl:value-of select="date:day-in-month (date:date ())"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="date:month-abbreviation (date:date ())"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="date:year (date:date ())"/>
    <xsl:text>, </xsl:text>
    <xsl:value-of select="date:time()"/>
  </div>
  <div class="footer-disclaimer">
    <div>
      This file was generated by <a
      href="http://planner.imendio.org/">Planner</a>.
    </div>
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


<!--/\___________________________________________________________________/\-->
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
      <h1 class="proj-title"><a name="project"><xsl:value-of select="@name"/></a></h1>
      <div class="proj-header">
        <xsl:if test="@company != ''">
          <div class="proj-company">
            <span class="header">Company: </span>
            <span class="value"><xsl:value-of select="@company"/></span>
          </div>
        </xsl:if>
        <xsl:if test="@manager != ''">
          <div class="proj-manager">
            <span class="header">Manager: </span>
            <span class="value"><xsl:value-of select="@manager"/></span>
          </div>
        </xsl:if>
        <div class="proj-start">
          <span class="header">Start Date: </span>
          <span class="value">
            <xsl:value-of select="substring ($projstart, 1, 10)"/>
          </span>
        </div>
        <div class="proj-end">
          <span class="header">End Date: </span>
          <span class="value">
            <xsl:value-of select="substring (date:add ($projstart,
                                        date:duration ($projlength)), 1, 10)"/>
          </span>
        </div>
        <div class="proj-duration">
          <span class="header">Duration: </span>
          <span class="value">
            <xsl:call-template name="mrproj-duration">
              <xsl:with-param name="duration" 
                select="substring (date:duration ($projlength), 2)"/>
            </xsl:call-template>
          </span>
        </div>
        <div class="proj-report">
          <span class="header">Report Date: </span>
          <span class="value">
            <xsl:value-of select="substring (date:date (), 1, 10)"/>
          </span>
        </div>
      </div>
      <div class="mainmenu">
        <h2>Menu</h2>
        <div class="item"><a href="#project">Project Info</a></div>
        <div class="item"><a href="#gantt">Gantt Chart</a></div>
        <div class="item"><a href="#milestones">Milestones</a></div>
        <div class="item"><a href="#tasks">Task List</a></div>
        <div class="item"><a href="#resources">Resources</a></div>
      </div><div style="clear: both"/>

      <!-- Defined in html1_gantt.xsl -->
      <xsl:call-template name="gantt"/> 

      <div class="separator"/>

      <!-- Defined in html1_milestones.xsl -->
      <xsl:call-template name="milestones"/> 

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



