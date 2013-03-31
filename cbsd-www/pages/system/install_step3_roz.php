<?php

function content_system_install_step3_roz()
{
 // required library
 activate_library('zfs');

 // call function
 $zpools = zfs_pool_list();

 // determine pool boot status
 $bootable = array();
 if (is_array($zpools) AND (!empty($zpools)))
  foreach ($zpools as $poolname => $zpool)
   if (strlen($bootfs = zfs_pool_getbootfs($poolname)) > 1)
    $bootable[$poolname] = $bootfs;

 // variables
 $dist = htmlentities(trim($_GET['dist']));
 $sysver = htmlentities(trim($_GET['sysver']));
 $override = (@isset($_GET['override'])) ? true : false;

 // visible classes
 $class_activebootfs = (!empty($bootable)) ? 'normal' : 'hidden';
 $class_noactivebootfs = (empty($bootable) AND !empty($zpools)) ? 
  'normal' : 'hidden';
 $class_nopools = (empty($zpools)) ? 'normal' : 'hidden';
 $class_override = ($override) ? 'normal' : 'hidden';

 // craft poollist table array
 $poollist = array();
 if (is_array($zpools) AND (!empty($zpools)))
  foreach ($zpools as $poolname => $zpool)
  {
   $pool_esc = htmlentities(trim($poolname));
   $poolstatus = ($zpool['status'] == 'ONLINE')
    ? '<b>'.$zpool['status'].'</b>'
    : '<b><span style="color:#a22">'.$zpool['status'].'</span></b>';

   // boot filesystem
   $bootfs = zfs_pool_getbootfs($poolname);
   if ($bootfs == false)
    $bootfs = '-';
   if (($bootfs != '-') AND (@strlen($bootfs) > 0))
    $bootstatus = 'Bootable';
   else
    $bootstatus = 'Not Bootable';
   $button = ($bootfs == '-') ? 'hidden' : 'normal';

   // pool link (clickable only if no active boot filesystems are found)
   if (($zpool['status'] != 'ONLINE') AND ($zpool['status'] != 'DEGRADED'))
    $poollink = $pool_esc;
   elseif (!@isset($bootable[$poolname]) AND !$override AND 
    $class_noactivebootfs != 'normal')
    $poollink = $pool_esc;
   else
    $poollink = '<a href="system.php?install&dist='.$dist.'&sysver='.$sysver
     .'&target='.urlencode($poolname).'">'.$pool_esc.'</a>';

   // add row to table array
   $poollist[] = array(
    'POOLLIST_POOLNAME'		=> $pool_esc,
    'POOLLIST_POOLLINK'		=> $poollink,
    'POOLLIST_POOLSTATUS'	=> $poolstatus,
    'POOLLIST_BOOTSTATUS'	=> $bootstatus,
    'POOLLIST_BOOTFS'		=> $bootfs,
    'POOLLIST_BUTTON'		=> $button
   );
  }

 // assemble active bootfs buttons which disable the bootfs property
 $abfs = '';
 foreach ($bootable as $poolname => $bootfs)
  $abfs .= '<input type="submit" name="disable_bootfs_'.urlencode($poolname).'"'
   .' value="Disable '.htmlentities($poolname).' bootfs" />';

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'		=> 'Install',
  'PAGE_TITLE'			=> 'Install (step 3)',
  'TABLE_INSTALL_POOLLIST'	=> $poollist,
  'CLASS_ACTIVEBOOTFS'		=> $class_activebootfs,
  'CLASS_NOACTIVEBOOTFS'	=> $class_noactivebootfs,
  'CLASS_NOPOOLS'		=> $class_nopools,
  'CLASS_OVERRIDE'		=> $class_override,
  'INSTALL_DIST'		=> $dist,
  'INSTALL_SYSVER'		=> $sysver,
  'INSTALL_ACTIVEBOOTFS'	=> $abfs
 );
 return $newtags;
}

function submit_install_disablebootfs()
{
 $url = 'system.php?install&dist='.@$_GET['dist'].'&sysver='.@$_GET['sysver'];
 $poolname = false;
 // scan POST vars for poolname
 foreach ($_POST as $name => $value)
  if (substr($name, 0, strlen('disablebootfs_')) == 'disablebootfs_')
   $poolname = trim(substr($name, strlen('disablebootfs_')));
 // sanitize
 $s = sanitize($poolname);
 if (!$s)
  friendlyerror('invalid pool name; cannot disable boot filesystem', $url); 
 // defer to dangerous command function
 dangerouscommand('/sbin/zpool set bootfs= '.$poolname, $url);
}

?>
