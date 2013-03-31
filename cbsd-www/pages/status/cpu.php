<?php

function content_status_cpu()
{

 // calculate CPU usage
 $cmd = trim(`/usr/bin/uptime`);
 $tmp = substr($cmd, strpos($cmd, 'up ')+3);
 $uptime = substr($tmp, 0, strpos($tmp, ','));
 $loadavg = trim(substr($cmd, strrpos($cmd, ':')+1));
 $tmp = substr($loadavg, 0, strpos($loadavg, ','));
 $cpuusage = ((double)$tmp * 100) . ' %';

 // top output
 exec('/usr/bin/top -b all', $result);
 $topoutput = implode(chr(10), $result);

 // refresh page every x seconds
 $refresh_sec = 2;
 page_refreshinterval($refresh_sec);

 // export new tags
 $newtags = array(
  'STATUS_CPUUSAGE'	=> $cpuusage,
  'STATUS_TOPOUTPUT'	=> $topoutput,
  'STATUS_REFRESH_SEC'	=> $refresh_sec
 );
 return $newtags;
}

?>
