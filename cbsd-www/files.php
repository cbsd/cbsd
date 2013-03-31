<?php

// import main lib
require('includes/main.php');

// generate tabs
$tabs = array(
 'Filesystems' => 'files.php',
 'File browser' => 'files.php?browse',
 'Snapshots' => 'files.php?snapshots',
 'Volumes' => 'files.php?zvol');
if (@$guru['preferences']['advanced_mode'] !== true)
 unset($tabs['ZVOLs']);

// select page
if (isset($_GET['query']))
 $content = content_handle('files', 'query');
elseif (isset($_GET['destroy']))
 $content = content_handle('files', 'destroy');
elseif (isset($_GET['browse']))
 $content = content_handle('files', 'filebrowse');
elseif (isset($_GET['snapshots']))
 $content = content_handle('files', 'snapshots');
elseif (isset($_GET['zvol']))
 $content = content_handle('files', 'zvol');
else
 $content = content_handle('files', 'filesystems');

// serve page
page_handle($content);

?>
