<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml">
<xsl:comment>

/* CSS Stylesheet for Planner HTML output.
 * 
 * Copyright (C) 2004 Imendio AB
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Daniel Lundin (daniel@edgewall.com)
 * Copyright (C) 2004 Chris Ladd (caladd@particlestorm.net)
 */

/*
 * Fonts 
 */
html,body,table { 
 font-family: Bitstream Vera Sans, helvetica, Arial, sans-serif;
 font-size: 12px;
 white-space: nowrap;
}

tr,td,th,table,font,span,div,h1,h2,h3 {
 font-family: Bitstream Vera Sans, helvetica, Arial, sans-serif;
}

h1 {
 font-size: 16px;
}

h2 {
 font-size: 12px;
 margin-bottom: 2px;
}

div.separator {
 margin: 1em;
}

/*
 * Header
 */
table.proj-header {
 border: 0;
 margin: 0;
}

table.proj-header .header {
 font-weight: bold;
}

/*
 * Footer
 */
.footer {
 margin-top: 50px;  
 padding-top: 2px;
 border-style: dotted;
 border-width: 1px 0 0 0;
 border-color: #999;
 font-size: 9px;
 text-align: right;
 clear: both;
 color: #666;
}

a:link, a:visited {
 text-decoration: none;
 color: #666;
}

a:hover[href] {
 text-decoration: underline;
}


/*
 * Tables
 */
table {
 display: table;
 border-collapse: collapse;	
 border-style: solid;
 border-width: 1px
 border-color: #000000;
 white-space: nowrap;
 margin: 2px;
 padding: 2px;
}

tr, td, th {
 white-space: nowrap;
 vertical-align: top;
}

tr.header {
 background-color: #aaaaaa;
 color: #ffffff;
}

tr.even {
 background-color: #eeeeee;
}

tr.odd {
 background-color: #ffffff;
}

th span, td span {
 margin-left: 8px;
 margin-right: 8px;
}

span.note {
 white-space: normal;
}

/*
 * Gantt
 */
div.gantt-empty-begin, div.gantt-empty-end, div.gantt-complete-done, div.gantt-complete-notdone, div.gantt-summary {
 clear: none;
 float: left; 
 height: 13px;
 margin-top: 1px;
 margin-bottom: 1px;
}

div.gantt-complete-done {
 background-color: #495f6b;
 height: 7px;
 margin-top: 3px
}

div.gantt-complete-notdone {
 background-color: #8db6cd;
 border-style: solid;
 border-width: 1px;
 border-color: #4d768d;
}

div.gantt-summary {
 height: 3px;
 margin-top: 3px;
 border-bottom: 2px dashed black;
}

div.gantt-empty-end {
 margin-left: 0;
}

span.gantt-milestone {
 font-size: 9px;
 color: #000000;
 vertical-align: middle;
 position: relative;
 margin-left: 0;
 padding-left: 0;
}

span.gantt-resources {
 margin-left: 4px;
}

th.gantt-day-header {
 margin: 0;
 padding: 0;
}

</xsl:comment>
</xsl:stylesheet>
