<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times">

<!--**************************************************************************
    *
    * html1_milestones.xsl: Display milestones
    *
    * Copyright (c)2003 Daniel Lundin
    * Copyright (c)2003 CodeFactory AB
    *
    *-->

<xsl:template name="milestones">
  <h2><a name="milestones">Project Milestones</a></h2>
  <table class="milestones" cellspacing="1" cellpadding="2" border="1">
    <tr class="milestones-hdr">
      <th align="left" class="milestones-hdr">Milestone</th>
      <th align="left" class="milestones-hdr">Date</th>
      <th align="left" class="milestones-hdr">Notes</th>
    </tr>
    <xsl:for-each select="//task[@type='milestone']">
      <xsl:sort data-type="number" 
        select="date:seconds (date:add (concat (substring (@start, 1, 4),'-',
                substring (@start, 5, 2),'-',
                substring (@start, 7, 2)), 
                date:duration (substring (@end, 10))))"
        order="descending"/>
      <xsl:variable name="mid" select="@id"/>
      <xsl:variable name="mdate">
        <xsl:call-template name="mrproj-parse-date">
          <xsl:with-param name="thedate" select="@start"/>
        </xsl:call-template>
      </xsl:variable>
      <tr>
        <td class="milestones-name" align="left" valign="middle">
          <xsl:value-of select="@name"/>
        </td>
        <td class="milestones-date" align="left" valign="middle">
          <xsl:value-of select="date:day-in-month ($mdate)"/> 
          <xsl:text> </xsl:text>
          <xsl:value-of select="date:month-abbreviation ($mdate)"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="date:year ($mdate)"/>
        </td>
        <td class="milestones-note" align="left" valign="top">
          <xsl:value-of select="@note"/>
        </td>
      </tr>
    </xsl:for-each>
  </table>
</xsl:template>



</xsl:stylesheet>