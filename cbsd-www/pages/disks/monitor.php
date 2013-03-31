<?php

function content_disks_monitor()
{
 activate_library('super');

 // filter
 $filter = @$_GET['filter'];
 if (strlen($filter) < 1)
  $filter = '^(gpt|label)\/';

 // execute gstat command (with elevated privileges)
 if ($filter == '-a')
  $result = super_execute('/usr/sbin/gstat -b -a');
 elseif ($filter == '-all')
  $result = super_execute('/usr/sbin/gstat -b');
 else
  $result = super_execute('/usr/sbin/gstat -b -f "'.$filter.'"');

 // refresh page in intervals
 $page_refresh = 2;
 if (!@isset($_GET['norefresh']))
  page_refreshinterval($page_refresh);
 $class_startrefreshing = (@isset($_GET['norefresh'])) ? 'normal' : 'hidden';
 $class_stoprefreshing = (@!isset($_GET['norefresh'])) ? 'normal' : 'hidden';

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'I/O monitor',
  'PAGE_TITLE'		=> htmlentities('I/O monitor'),
  'CLASS_STOPREFRESH'	=> $class_stoprefreshing,
  'CLASS_STARTREFRESH'	=> $class_startrefreshing,
  'MONITOR_FILTER'	=> htmlentities($filter),
  'MONITOR_OUTPUT'	=> $result['output_str'],
  'MONITOR_REFRESH'	=> $page_refresh
 );
 return $newtags;
}

?>
