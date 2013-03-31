<?php

function content_system_update()
{
 global $guru;

 // required libraries
 activate_library('guru');

 // call functions
 $remote = guru_readremotefile($guru['remote_version_url']);

 // check if remote file was retrievable or not
 if (($remote == false) OR (!is_array($remote)))
  error('this page requires internet access - please check your network settings!');

 // retrieve stable and experimental version from remote file
 // bail out if remote configuration file version differs from the script
 if ($remote['version'] != $guru['remote_update_version'])
  error('Version mismatch -- your version is too old update from website');
 // now process stable/experimental version
 if ((@strlen($remote['stable']) < 1) OR
  (@strlen($remote['stable']) > 256))
  $stableversion = '<span class="warning">ERROR:</span> could not '
   .'fetch version"';
 else
  $stableversion = htmlentities($remote['stable']);
 if ((@strlen($remote['experimental']) < 1) OR
  (@strlen($remote['experimental']) > 256))
  $expversion = '<span class="warning">ERROR:</span> could not '
   .'fetch version"';
 else
  $expversion = htmlentities($remote['experimental']);

 // submit buttons
 $ver = $guru['product_version_string'];
 $stablesubmit = ($remote['stable'] == $ver) ? 'disabled="disabled"' : '';
 $expsubmit = ($remote['experimental'] == $ver) ? 'disabled="disabled"' : '';

 // versions
 $stable = trim($remote['stable']);
 $exp = trim($remote['experimental']);

 // display message when update available
 $class_update = 'hidden';
 $class_update_exp = 'hidden';
 $class_running_exp = 'hidden';
 $class_running_future = 'hidden';
 $class_noupdate = 'hidden';
 if ($stable == $ver)
  $class_noupdate = 'normal';
 elseif ($exp == $ver)
  $class_running_exp = 'normal';
 else
 {
  for ($i = 0; $i < strlen($ver); $i++)
  {
   $s = @$stable{$i};
   $e = @$exp{$i};
   $v = @$ver{$i};
   if ($s == '')
   {
    $class_update = 'normal';
    break;
   }
   elseif ($e == '')
   {
    $class_update_exp = 'normal';
    break;
   }
   elseif (($v == $s) AND ($v != $e))
   {
    $class_update = 'normal';
    break;
   }
   elseif (($v == $e) AND ($v != $s))
   {
    $class_update_exp = 'normal';
    break;
   }
   elseif (($v != $s) AND ($v != $e))
   {
    $class_running_future = 'normal';
    break;
   }
  }
 }

 // craft new tags
 $newtags = array(
  'PAGE_ACTIVETAB'			=> 'Update',
  'PAGE_TITLE'				=> 'Update',
  'CLASS_UPDATE'			=> $class_update,
  'CLASS_UPDATE_EXP'			=> $class_update_exp,
  'CLASS_RUNNING_EXP'			=> $class_running_exp,
  'CLASS_RUNNING_FUTURE'		=> $class_running_future,
  'CLASS_NOUPDATE'			=> $class_noupdate,
  'SYSTEM_UPDATE_STABLEVERSION'		=> $stableversion,
  'SYSTEM_UPDATE_STABLESUBMIT'		=> $stablesubmit,
  'SYSTEM_UPDATE_EXPVERSION'		=> $expversion,
  'SYSTEM_UPDATE_EXPSUBMIT'		=> $expsubmit
 );
 return $newtags;
}

function submit_system_update()
{
 // required library
 activate_library('guru');

 /* update to stable branch */
 if (@isset($_POST['update_to_stable']))
  guru_update_webgui('stable');
 /* update to experimental branch */
 if (@isset($_POST['update_to_exp']))
  guru_update_webgui('experimental');
 /* update by HTTP file upload */
 if ($_FILES['import_webgui']['size'] > 0)
  guru_update_webgui('upload');

 // unhandled redirect
 redirect('system.php?update');
}

?>
