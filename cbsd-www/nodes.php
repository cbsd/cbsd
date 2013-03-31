<?php

// import main lib
require('includes/main.php');

// navtabs
$tabs = array(
 'List' => 'nodes.php',
// 'Bandwidth monitor' => 'network.php?monitor',
 'Create' => 'nodes.php?create',
 'Repo' => 'nodes.php?repo');
//if (@$guru['preferences']['advanced_mode'] !== true)
// unset($tabs['Link aggregation']);

// select page
if (@isset($_GET['monitor']))
 $content = content_handle('nodes', 'monitor');
elseif (@isset($_GET['firewall']))
 $content = content_handle('nodes', 'create');
elseif (@isset($_GET['lagg']))
 $content = content_handle('nodes', 'repo');
elseif (@isset($_GET['query']))
 $content = content_handle('nodes', 'networkquery');
else
 $content = content_handle('nodes', 'nodes');

// serve page
page_handle($content);

?>
