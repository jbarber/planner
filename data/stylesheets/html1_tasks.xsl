<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times">

<!--**************************************************************************
    *
    * html1_tasks.xsl: Display tasks in a table with progress bars
    *
    * Copyright (c)2003 Daniel Lundin
    * Copyright (c)2003 CodeFactory AB
    *
    *-->


<xsl:template match="tasks">
  <xsl:variable name="hasproperties" 
    select="boolean (count(//task/properties/property[@value!='']))"/>
  <xsl:variable name="hasnotes"
    select="boolean (count(//task[@note!='']))"/>
  <!-- Hierarcichal task indentation in pixels -->

  <h2><a name="tasks">Tasks</a></h2>
  <table class="tasks" cellspacing="1" cellpadding="2" border="1">
    <tr class="tasks-hdr">
      <th align="left" class="tasks-hdr">Name</th>
      <th align="left" class="tasks-hdr">Progress</th>
      <th align="left" class="tasks-hdr">Start</th>
      <th align="left" class="tasks-hdr">End</th>
      <th align="left" class="tasks-hdr">Work</th>
      <th align="left" class="tasks-hdr">Resources</th>
      <xsl:if test="$hasproperties">
        <th align="left" class="tasks-hdr">Properties</th>
      </xsl:if>
      <xsl:if test="$hasnotes">
        <th align="left" class="tasks-hdr">Notes</th>
      </xsl:if>
    </tr>
    <xsl:for-each select="//task">
      <xsl:sort data-type="number" 
        select="date:seconds (date:add (concat (substring (@end, 1, 4),'-',
                substring (@end, 5, 2),'-',
                substring (@end, 7, 2)), 
                date:duration (substring (@end, 10))))"
        order="descending"/>
        <xsl:variable name="tid" select="@id"/>
        <!-- Make a proper XSL date value from the start-date attribute -->
        <xsl:variable name="start_date">
          <xsl:call-template name="mrproj-parse-date">
            <xsl:with-param name="thedate" select="@start"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="end_date">
          <xsl:call-template name="mrproj-parse-date">
            <xsl:with-param name="thedate" select="@end"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="indent" select="count (ancestor::task)"/>
        <xsl:variable name="tname">        
        </xsl:variable>
        
        <xsl:choose>
          <xsl:when test="task">
            <tr class="task-parent" align="left" valign="top">
              <td class="task-parent-name" colspan="8">
                <a name="task{@id}" 
                   style="margin-left: {$indent*$task-indent-pixels}px">
                  <xsl:value-of select="@name"/>
                </a>
              </td>
            </tr>
          </xsl:when>
          <xsl:when test="@type='milestone'">
            <tr class="task-milestone" align="left" valign="top">
              <td class="task-milestone-row" colspan="2">

                <a name="task{@id}" 
                   style="margin-left: {$indent*$task-indent-pixels}px">
                  <xsl:value-of select="@name"/>
                </a>
              </td>
              <td colspan="7">
                <xsl:variable name="st" select="substring ($start_date, 12, 5)"/>
                <span class="datetime">
                  <span class="date">
                    <xsl:value-of select="substring ($start_date, 1, 10)"/>
                  </span>
                  <xsl:if test="$st">
                    <span class="time">
                      <xsl:value-of select="$st"/>
                    </span>
                  </xsl:if>
                </span>
              </td>
            </tr>
          </xsl:when>
          <xsl:otherwise>

            <tr class="task" align="left" valign="top">
              <td class="task-name">
                <span class="task-indent" />
                <a name="task{@id}" style="margin-left: {$indent*$task-indent-pixels}px">
                  <xsl:value-of select="@name"/>
                </a>
              </td>

              <td class="task-progress">
                <span class="task-progress-done" 
                  style="width: {@percent-complete}px;">
                  <xsl:text> </xsl:text>
                </span>
                <span class="task-progress-notdone" 
                  style="width: {100 - @percent-complete}px;">
                  <xsl:text> </xsl:text>
                </span> 

                <span class="task-progress-caption">
                  <xsl:value-of select="@percent-complete"/>%
                </span>
              </td>
              <td class="task-date">
                <xsl:variable name="st" select="substring ($start_date, 12, 5)"/>
                <span class="datetime">
                  <span class="date">
                    <xsl:value-of select="substring ($start_date, 1, 10)"/>
                  </span>
                  <xsl:if test="$st">
                    <span class="time">
                      (<xsl:value-of select="$st"/>)
                    </span>
                  </xsl:if>
                </span>
              </td>
              <td class="task-date">
                <xsl:variable name="et" select="substring ($end_date, 12, 5)"/>
                <span class="datetime">
                  <span class="date">
                    <xsl:value-of select="substring ($end_date, 1, 10)"/>
                  </span>
                  <xsl:if test="$et">
                    <span class="time">
                      (<xsl:value-of select="$et"/>)
                    </span>
                  </xsl:if>
                </span>
              </td>
              <td class="task-work">
                <xsl:call-template name="mrproj-duration">
                  <xsl:with-param name="duration" select="substring (date:duration ((3600 * @work) div 1200),2)"/>
                </xsl:call-template>
              </td>
              <td class="task-resources">
                <xsl:for-each select="/project/allocations/allocation[@task-id=$tid]">
                  <xsl:variable name="rid" select="@resource-id"/>
                  <xsl:variable name="res"
                    select="/project/resources/resource[@id=$rid]"/>
                  <div class="task-resource">
                    <xsl:choose>
                      <xsl:when test="$res/@email!=''">
                        <a href="mailto:{$res/@email}">
                          <xsl:value-of select="$res/@name"/>
                        </a>
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:value-of select="$res/@name"/>
                      </xsl:otherwise>
                    </xsl:choose>
                  </div>
                </xsl:for-each>
              </td>
              <xsl:if test="$hasproperties">
                <td class="task-properties">
                  <xsl:for-each select="properties/property">
                    <xsl:call-template name="property"/>          
                  </xsl:for-each>
                </td>
              </xsl:if>
              <xsl:if test="$hasnotes">
                <td class="task-notes">
                  <xsl:value-of select="@note"/>
                </td>
              </xsl:if>
            </tr>
          </xsl:otherwise>
        </xsl:choose>



    </xsl:for-each>
  </table>
</xsl:template>

</xsl:stylesheet>