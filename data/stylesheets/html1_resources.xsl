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
    * Copyright (C) 2004 Imendio AB
    * Copyright (c) 2003 Daniel Lundin
    * Copyright (c) 2003 CodeFactory AB
    * Copyright (c) 2004 Chris Ladd (caladd@particlestorm.net)
    *
    *-->

<xsl:template match="resources">
  <xsl:variable name="hasproperties" 
    select="boolean (count(//resource/properties/property[@value!='']))"/>
  <xsl:variable name="hasnotes"
    select="boolean (count(//resource[@note!='']))"/>
  
  <h2><a name="resources">Resources</a></h2>

  <table cellspacing="0" cellpadding="0" border="1" width="100%">
    <tr class="header" align="left">
      <th><span>Name</span></th>
      <th><span>Short name</span></th>
      <th><span>Type</span></th>
      <th><span>Group</span></th>
      <th><span>Email</span></th>
      <th><span>Cost</span></th>
      <xsl:if test="$hasnotes">
        <th><span>Notes</span></th>
      </xsl:if>
    </tr>

    <xsl:for-each select="../resources/resource">
	  <xsl:sort select="@type"/>
	  <xsl:sort select="@name"/>
          
      <xsl:call-template name="resource-row">
        <xsl:with-param name="hasnotes" select="$hasnotes"/>
	  </xsl:call-template>
          
	</xsl:for-each>
  </table>
</xsl:template>


<xsl:template name="resource-row">
  <xsl:param name="hasnotes"/>
        <xsl:variable name="rid" select="@id"/>
        <xsl:variable name="gid" select="@group"/>
        
		<xsl:variable name="rowclass">
          <xsl:choose>
            <xsl:when test="(position() mod 2) = 0">even</xsl:when>
            <xsl:otherwise>odd</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
      
	    <tr class="{$rowclass}">
          <td>
            <a name="res-{@id}">
			  <span>
			    <xsl:value-of select="@name"/>
			  </span>
			</a>
          </td>
	      <td>
            <span>
			  <xsl:value-of select="@short-name"/>
			</span>
          </td>
          <td>
            <span>
			  <xsl:choose>
                <xsl:when test="@type = 1">Work</xsl:when>
                <xsl:otherwise>Material</xsl:otherwise>
              </xsl:choose>
			</span> 
          </td>
          <td>
		    <span>
			  <xsl:value-of select="../../resource-groups/group[@id=$gid]/@name"/>
		    </span>
		  </td>
		  <td>
            <a href="mailto:{@email}">
              <span>
			    <xsl:value-of select="@email"/>
			  </span>
            </a>
          </td>
         <td align="right">
           <span>
		     <xsl:value-of select="@std-rate"/>
		   </span>
         </td>
         <xsl:if test="$hasnotes">
           <td>
             <span>
			   <xsl:value-of select="@note"/>
			 </span>
           </td>
         </xsl:if>
       </tr>
</xsl:template>

</xsl:stylesheet>
