<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml">
<xsl:comment>

/* CSS Stylesheet for Planner HTML output.
 * 
 * Copyright (C) 2004-2005 Imendio AB
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Daniel Lundin (daniel@edgewall.com)
 * Copyright (C) 2004 Chris Ladd (caladd@particlestorm.net)
 */

/*
 * Fonts 
 */
html,body,table { 
 font-family: "Bitstream Vera Sans", helvetica, Arial, sans-serif;
 font-size: 12px;
 white-space: nowrap;
}

tr,td,th,table,font,span,div,h1,h2,h3 {
 font-family: "Bitstream Vera Sans", helvetica, Arial, sans-serif;
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
 width: auto;
}

table.proj-header .header {
 font-weight: bold;
}

/*
 * Footer
 */
.footer {
 float: left;
 width: 100%;
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
 * Layout
 */

.gantt, .gantt-tasklist, .gantt-chart, .tasklist, .resourcelist {
 float: left;
}

.gantt-tasklist, .gantt-chart, .tasklist-table, .resourcelist-table {
 border-style: solid;
 border-width: 1px;
 border-color: #aaa;
}

.gantt-tasklist, .gantt-chart, .tasklist, .resourcelist {
 overflow: auto;
}

.gantt, .tasklist, .resourcelist {
  clear: both;
  width: 100%;
}

.gantt-tasklist {
 border-width: 1px 0px 1px 1px;
 width: 30%;
}

.gantt-chart {
 border-color: #aaa #aaa #aaa #fff;
 width: 69.5%;
}

.tasklist, .resourcelist {
 clear: left;
}

table {
 width: 100%;
 border-collapse: collapse;
 border-style: none;
 border-color: #fff;
 white-space: nowrap;
 margin: 0;
}

tr, td, th {
 white-space: nowrap;
 vertical-align: top;
 padding-top: 1px;
 padding-bottom: 1px;
}

th {
 vertical-align: top;
}

tr {
 height: 1.5em;
}

tr.header {
 background-color: #aaa;
 color: #fff;
}

tr.even {
 background-color: #eee;
}

tr.odd {
 background-color: #fff;
}

th span, td span {
 margin-left: 6px;
 margin-right: 6px;
}

th.note {
 min-width: 20em;
}

td.note {
 white-space: normal;
}

/*
 * Gantt
 */
div.gantt-empty-begin, div.gantt-empty-end, div.gantt-complete-done, div.gantt-complete-notdone, div.gantt-summary {
 overflow: hidden;
 clear: none;
 float: left; 
 height: 0.75em;
 margin-top: 0.15em;
 margin-bottom: 0;
}

div.gantt-complete-done {
 background-color: #495f6b;
 height: 0.75em;
 margin-top: 0;
 margin-bottom: 0;
}

div.gantt-complete-notdone {
 background-color: #8db6cd;
 border-style: solid;
 border-width: 1px;
}

div.gantt-summary {
 height: 0.3em;
 margin-top: 0.25em;
 border-bottom: 2px dashed #000;
}

div.gantt-empty-end {
 margin-left: 0;
}

div.gantt-milestone {
 float: left;
 font-size: 0.9em;
 color: #000000;
 position: relative;
 margin-left: 0;
 margin-right: 0;
}

div.gantt-resources {
 float: left;
 margin-left: 0.5em;
 white-space: nowrap;
}

th.gantt-1day-header {
  width: 19px;
}

th.gantt-2day-header {
  width: 39px;
}

th.gantt-3day-header {
  width: 59px;
}

th.gantt-4day-header {
  width: 79px;
}

th.gantt-5day-header {
  width: 99px;
}

th.gantt-6day-header {
  width: 119px;
}

th.gantt-week-header, .gantt-resources {
 width: 139px;
}

th.gantt-day-header {
 margin: 0;
 padding-top: 1px;
 padding-bottom: 1px;
 width: 19px;
}

</xsl:comment>
</xsl:stylesheet>
