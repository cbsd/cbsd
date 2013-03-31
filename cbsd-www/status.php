<?php

// import main lib
require('includes/main.php');

// set navtabs
$tabs = array(
 'Status'		=> 'status.php',
 'Processor usage'	=> 'status.php?cpu',
 'Memory usage'		=> 'status.php?memory',
 'System log'		=> 'status.php?log',
 'Release information'	=> 'status.php?release'
);
// hide advanced tabs unless advanced_mode is set
if (@$guru['preferences']['advanced_mode'] !== true)
 unset($tabs['Logs']);

// select page
if (@isset($_GET['cpu']))
 $content = content_handle('status', 'cpu');
elseif (@isset($_GET['memory']))
 $content = content_handle('status', 'memory');
elseif (@isset($_GET['log']))
 $content = content_handle('status', 'log');
elseif (@isset($_GET['release']))
 $content = content_handle('status', 'release');
else
 $content = content_handle('status', 'status');

// serve page
page_handle($content);

?>
