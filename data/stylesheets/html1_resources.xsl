<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml"
             xmlns:date="http://exslt.org/dates-and-times">

<!--**************************************************************************
    *
    * html1_resources.xsl: Display resources in a table
    *
    * Copyright (c)2003 Daniel Lundin
    * Copyright (c)2003 CodeFactory AB
    *
    *-->

<xsl:template match="resources">
  <xsl:variable name="gid_default"
    select="../resource-groups/@default_group"/>
  <xsl:variable name="hasproperties" 
    select="boolean (count(//resource/properties/property[@value!='']))"/>
  <xsl:variable name="hasnotes"
    select="boolean (count(//resource[@note!='']))"/>

  
  <h2><a name="resources">Resources</a></h2>


  <!-- Resource Groups -->
  <xsl:variable name="default-gid" select="../resource-groups/@default_group"/>

  <table class="resources" cellspacing="1" cellpadding="2" border="1">
    <tr class="resources-hdr">
      <th align="left" class="res-hdr">Name</th>
      <th align="left" class="res-hdr">Type</th>
      <th align="left" class="res-hdr">Email</th>
      <th align="left" class="res-hdr">Tasks</th>
      <xsl:if test="$hasproperties">
        <th align="left" class="res-hdr">Properties</th>
      </xsl:if>
      <xsl:if test="$hasnotes">
        <th align="left" class="res-hdr">Note</th>
      </xsl:if>
    </tr>

    <xsl:choose>
      <xsl:when test="$default-gid">
        <xsl:for-each select="../resource-groups/group">
          <xsl:sort select="@name"/>
          <xsl:variable name="gid" select="@id"/>
          
          <tr class="resource-group" align="left" valign="top">
            <td class="res-group-name">
              <xsl:value-of select="@name"/>
            </td>
            <td class="res-group-admin" colspan="3">
              Admin: <xsl:value-of select="@admin-name"/>
            <xsl:if test="@admin-email!=''">
              &lt;<a href="mailto:{@admin-email}">
              <xsl:value-of select="@admin-email"/></a>&gt;
            </xsl:if>
            <xsl:if test="@admin-phone!=''">
              (<xsl:value-of select="@admin-phone"/>)
            </xsl:if>
          </td>
        </tr>
        <xsl:for-each select="../../resources/resource[@group=$gid or (@group=''
                              and $gid=$default-gid)]">
          <xsl:sort select="@type"/>
          <xsl:sort select="@name"/>
          
          <xsl:call-template name="resource-row">
            <xsl:with-param name="hasproperties" select="$hasproperties"/>
            <xsl:with-param name="hasnotes" select="$hasnotes"/>
          </xsl:call-template>
          
        </xsl:for-each>
      </xsl:for-each>
    </xsl:when>

    <!-- There are no resource groups -->
    <xsl:otherwise>
        <xsl:for-each select="../resources/resource">
          <xsl:sort select="@type"/>
          <xsl:sort select="@name"/>
          
          <xsl:call-template name="resource-row">
            <xsl:with-param name="hasproperties" select="$hasproperties"/>
            <xsl:with-param name="hasnotes" select="$hasnotes"/>
          </xsl:call-template>
          
        </xsl:for-each>
    </xsl:otherwise>
  </xsl:choose>

  </table>
</xsl:template>


<xsl:template name="resource-row">
  <xsl:param name="hasproperties"/>
  <xsl:param name="hasnotes"/>
        <xsl:variable name="rid" select="@id"/>
        <xsl:variable name="gid" select="@group"/>
        <tr class="resource" align="left" valign="top">
          <td class="res-name">
            <span class="res-name">
              <a name="res-{@id}"><xsl:value-of select="@name"/></a>
            </span>
          </td>
          <td class="res-type">
            <xsl:choose>
              <xsl:when test="@type = 1">Worker</xsl:when>
              <xsl:otherwise>Material</xsl:otherwise>
            </xsl:choose>
          </td>
          <td class="res-email">
            <a href="mailto:{@email}">
              <xsl:value-of select="@email"/>
            </a>
          </td>
         <td class="res-tasks">
           <ul>
             <xsl:for-each select="../../allocations/allocation[@resource-id=$rid]">
               <xsl:variable name="tid" select="@task-id"/>
               <xsl:variable name="task" select="../../tasks//task[@id=$tid]"/>
               <xsl:variable name="tstyle">res-tasks-task<xsl:choose>
                   <xsl:when 
                     test="$task/@percent-complete &gt;= 100">-done</xsl:when>
                   <xsl:when 
                     test="$task/@percent-complete &gt; 0">-ongoing</xsl:when>
                 </xsl:choose>
               </xsl:variable>
               <li class="{$tstyle}">
                 <!-- <a href="#task-{$tid}"> -->
                 <xsl:value-of select="$task/@name"/>
                 <!-- (<xsl:value-of select="$task/@percent-complete"/>%) -->
                 <!-- </a> -->
               </li>
             </xsl:for-each>
           </ul>
         </td>
         <xsl:if test="$hasproperties">
           <td class="res-properties">
             <xsl:for-each select="properties/property">
               <xsl:call-template name="property"/>          
             </xsl:for-each>
           </td>
         </xsl:if>
         <xsl:if test="$hasnotes">
           <td class="res-note">
             <xsl:value-of select="@note"/>
           </td>
         </xsl:if>
       </tr>
</xsl:template>

</xsl:stylesheet>