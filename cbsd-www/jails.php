<?php

// import main lib
require('includes/main.php');

// navtabs
$tabs = array(
 'List' => 'jails.php',
// 'Bandwidth monitor' => 'network.php?monitor',
 'Create' => 'jails.php?create',
 'Repo' => 'jails.php?repo');
//if (@$guru['preferences']['advanced_mode'] !== true)
// unset($tabs['Link aggregation']);

// select page
if (@isset($_GET['monitor']))
 $content = content_handle('jails', 'monitor');
elseif (@isset($_GET['firewall']))
 $content = content_handle('jails', 'create');
elseif (@isset($_GET['lagg']))
 $content = content_handle('jails', 'repo');
elseif (@isset($_GET['query']))
 $content = content_handle('jails', 'jailsquery');
else
 $content = content_handle('jails', 'jails');

// serve page
page_handle($content);

?>
