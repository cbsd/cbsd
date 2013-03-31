<?php

function content_system_install_step3_emb()
{
 // required library
 activate_library('disk');
 activate_library('zfs');

 // call functions
 $disks = disk_detect_physical();
 $dmesg = disk_detect_dmesg();
 $labels = disk_detect_label();
 $gpart = disk_detect_gpart();
 $members = zfs_pool_memberdisks();

 // variables
 $dist = trim($_GET['dist']);
 $sysver = trim($_GET['sysver']);

 // process disklist table
 $disklist = array();
 if (!is_array($disks))
  error('Could not find any physical disks on your system!');
 foreach ($disks as $disk)
 {
  if (@$labels[$disk['disk_name']])
   $label = 'label/'.$labels[$disk['disk_name']];
  else
   $label = $disk['disk_name'];
  if ($poolname = zfs_pool_ismemberdisk($label, $members, false))
  {
   $disklink = htmlentities($disk['disk_name']);
   $status = 'Member of pool <b>'.htmlentities($poolname).'</b>';
  }
  else
  {
   $disklink = '<a href="system.php?install&dist='.$dist.'&sysver='
    .$sysver.'&target='.$disk['disk_name'].'">'
    .htmlentities($disk['disk_name']).'</a>';
   $status = '<i>free</i>';
  }

  // add table row
  $disklist[] = array(
   'DISKLIST_DISKLINK'		=> $disklink,
   'DISKLIST_STATUS'		=> $status,
   'DISKLIST_SIZEHUMAN'		=> sizehuman($disk['mediasize'], 1),
   'DISKLIST_SIZEBINARY'	=> sizebinary($disk['mediasize'], 1),
   'DISKLIST_DISKIDENTIFY'	=> @$dmesg[$disk['disk_name']]
  );
 }

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'		=> 'Install',
  'PAGE_TITLE'			=> 'Install (step 3)',
  'TABLE_INSTALL_DISKLIST'	=> $disklist,
  'INSTALL_DIST'                => $dist,
  'INSTALL_SYSVER'              => $sysver
 );
 return $newtags;
}

?>
