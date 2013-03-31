<?php

// import main lib
require('includes/main.php');

// navtabs
$tabs = array(
 'Interfaces' => 'network.php',
// 'Bandwidth monitor' => 'network.php?monitor',
 'Firewall' => 'network.php?firewall',
 'Link aggregation' => 'network.php?lagg');
if (@$guru['preferences']['advanced_mode'] !== true)
 unset($tabs['Link aggregation']);

// select page
if (@isset($_GET['monitor']))
 $content = content_handle('network', 'monitor');
elseif (@isset($_GET['firewall']))
 $content = content_handle('network', 'firewall');
elseif (@isset($_GET['lagg']))
 $content = content_handle('network', 'lagg');
elseif (@isset($_GET['query']))
 $content = content_handle('network', 'networkquery');
else
 $content = content_handle('network', 'network');

// serve page
page_handle($content);

?>
