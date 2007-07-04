<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0"
              xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                  xmlns="http://www.w3.org/1999/xhtml">
<xsl:comment>

/* IE specific overrides to compensate for the different box model used by IE
 * (see http://en.wikipedia.org/wiki/Internet_Explorer_box_model_bug)
 */

/*
div.gantt-empty-begin, div.gantt-empty-end, div.gantt-complete-done, div.gantt-complete-notdone, div.gantt-summary {
 height: 1.75em;
}

div.gantt-complete-done {
 height: 0.75em;
}

div.gantt-summary {
 height: 0.3em;
}
*/
th.gantt-1day-header {
  width: 20px;
}

th.gantt-2day-header {
  width: 40px;
}

th.gantt-3day-header {
  width: 60px;
}

th.gantt-4day-header {
  width: 80px;
}

th.gantt-5day-header {
  width: 100px;
}

th.gantt-6day-header {
  width: 120px;
}

th.gantt-week-header {
 width: 140px;
}

th.gantt-day-header {
 width: 20px;
}

</xsl:comment>
</xsl:stylesheet>
