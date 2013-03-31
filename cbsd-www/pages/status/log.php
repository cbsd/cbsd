<?php

function content_status_log()
{
 // tabbar
 $tabbar = array(
  'kernel' => 'Kernel',
  'system' => 'System',
  'webserver' => 'Webserver',
 );
 $tabbar_url = 'status.php?log';

 // select tab
 if (isset($_GET['system']))
 {
  $tabbar_tab = '&system';
  $log_name = 'System message log';
  $log_output = htmlentities(trim(`cat /var/log/messages`));
 }
 elseif (isset($_GET['webserver']))
 {
  $tabbar_tab = '&webserver';
  $log_name = 'Webserver log';
  // TODO: make lighttpd/apache independent
  $log_output = htmlentities(trim(`cat /var/log/lighttpd/error.log`));
 }
 else
 {
  // default tab:
  $tabbar_tab = '';
  $log_name = 'Kernel log';
  $log_output = htmlentities(trim(`dmesg`));
 }

 // add javascript code to scroll to bottom of pre scrollbox
 page_register_headelement('
  <script type="text/javascript">
   window.onload=function() {
    var objDiv = document.getElementById("status_logbox");
    objDiv.scrollTop = objDiv.scrollHeight;
   };
  </script>'
 );

 // export new tags
 return @array(
  'PAGE_ACTIVETAB'	=> 'System log',
  'PAGE_TITLE'		=> 'System log',
  'PAGE_TABBAR'		=> $tabbar,
  'PAGE_TABBAR_URL'	=> $tabbar_url,
  'PAGE_TABBAR_URLTAB'	=> $tabbar_url . $tabbar_tab,
  'LOG_NAME'		=> $log_name,
  'LOG_OUTPUT'		=> $log_output
 );
}

?>
