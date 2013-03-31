<?php

// import main lib
require('includes/main.php');

// navtabs
$tabs = array(
 'Preferences' => 'system.php?pref',
 'Install' => 'system.php?install',
 'Tuning' => 'system.php?tuning',
 'Command line' => 'system.php?cli',
 'Update' => 'system.php?update',
 'Export' => 'system.php?export',
 'Shutdown' => 'system.php?shutdown'
);

// hide certain tabs when not enabled
if (@$guru['preferences']['advanced_mode'] !== true)
{
 unset($tabs['Tuning']);
 unset($tabs['Export']);
}

// select page
if (@isset($_GET['pref']))
 $content = content_handle('system', 'preferences');
elseif (@isset($_GET['install']))
{
 // different content for each installation step
 if (@isset($_GET['progress']))
  $content = content_handle('system', 'install_progress');
 elseif (@isset($_GET['startinstall']))
  $content = content_handle('system', 'install_submit');
 elseif (@isset($_GET['target']))
  $content = content_handle('system', 'install_step4');
 elseif (@isset($_GET['sysver']) AND @$_GET['dist'] == 'rootonzfs')
  $content = content_handle('system', 'install_step3_roz');
 elseif (@isset($_GET['sysver']) AND @$_GET['dist'] == 'embedded')
  $content = content_handle('system', 'install_step3_emb');
 elseif (@isset($_GET['dist']))
  $content = content_handle('system', 'install_step2');
 else
  $content = content_handle('system', 'install_step1');
}
elseif (@isset($_GET['tuning']))
 $content = content_handle('system', 'tuning');
elseif (@isset($_GET['cli']))
 $content = content_handle('system', 'cli');
elseif (@isset($_GET['root']))
 $content = content_handle('system', 'root');
elseif (@isset($_GET['update']))
 $content = content_handle('system', 'update');
elseif (@isset($_GET['export']))
 $content = content_handle('system', 'export');
elseif (@isset($_GET['shutdown']))
 $content = content_handle('system', 'shutdown');
elseif (@isset($_GET['activation']))
 $content = content_handle('system', 'activation');
else
 redirect_url('system.php?pref');

// serve page
page_handle($content);

?>
