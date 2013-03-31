<?php

// import main lib
require('includes/main.php');

// navtabs
$tabs = array(
 'Service panel'	=> 'services.php',
 'Manage'		=> 'services.php?manage',
 'Internal'		=> 'services.php?internal',
 'Install'		=> 'services.php?install',
);

/*
old tabs:
$tabs = array(
 'Services' => 'services.php',
 'OpenSSH' => 'services.php?ssh',
 'Samba' => 'services.php?samba',
 'NFS' => 'services.php?nfs',
// 'FTP' => 'services.php?ftp',
 'iSCSI' => 'services.php?iscsi');
*/

// select page
if (@isset($_GET['manage']) AND @isset($_GET['query']))
 $content = content_handle('services', 'query');
elseif (@isset($_GET['manage']))
 $content = content_handle('services', 'manage');
elseif (@isset($_GET['internal']))
 $content = content_handle('services', 'internal');
elseif (@isset($_GET['install']))
 $content = content_handle('services', 'install');
elseif (@isset($_GET['panel']))
{
 // handle panels directly
 // required library
 activate_library('service');
 // call function
 service_panel_handle($_GET['panel']);
 // error if not handled
 error('Panel function did not terminate!');
}
else
 $content = content_handle('services', 'services');

// serve content
page_handle($content);

?>
