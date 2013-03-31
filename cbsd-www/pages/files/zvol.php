<?php

function content_files_zvol()
{
 // required library
 activate_library('html');
 activate_library('zfs');

 // call functions
 $zvols = zfs_filesystem_volumes();
// $filesystems = zfs_filesystem_list();

 // retrieve queried volume
 $queryzvol = (@$_GET['zvol']) ? $_GET['zvol'] : '';

 // hide query div if no volume is queried
 $queryhidden = ($queryzvol) ? '' : 'class="hidden"';

 // filesystem selectbox
 $fs = html_zfsfilesystems();

 // craft zvol table
 $volumes = array();
 foreach ($zvols as $zvolname => $zvol)
 {
  $activerow = ($zvolname == $queryzvol AND $zvolname) 
   ? 'class="activerow"' : '';
  $volumes[] = array(
   'ZVOL_ACTIVEROW'	=> $activerow,
   'ZVOL_NAME'		=> $zvolname,
   'ZVOL_SIZEBINARY'	=> sizebinary($zvol['diskinfo']['mediasize']),
   'ZVOL_SIZEBYTES'	=> $zvol['diskinfo']['mediasize'],
   'ZVOL_REFER'		=> htmlentities($zvol['refer']),
   'ZVOL_USED'		=> htmlentities($zvol['used']),
   'ZVOL_SIZESECTOR'	=> (int)$zvol['diskinfo']['sectorsize'],
  );
 }

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'Volumes',
  'PAGE_TITLE'		=> 'ZFS Volumes',
  'TABLE_ZVOL_VOLUMES'	=> $volumes,
  'ZVOL_FILESYSTEMS'	=> $fs,
  'ZVOL_QUERYHIDDEN'	=> $queryhidden,
  'ZVOL_QUERYNAME'	=> $queryzvol
 );
 return $newtags;
}

function submit_zvol_create()
{
 // sanitize
 $s = sanitize(@$_POST['zvol_name'], null, $volname, 32);

 // variables
 $url		= 'files.php?zvol';
 $fs            = @$_POST['zvol_filesystem'];
 $path		= $fs.'/'.$volname;
 $size_gib      = @$_POST['zvol_size'];

 // sanity check
 if (!$s)
  friendlyerror('please enter a valid name for your ZFS volume consisting of '
   .'a maximum of 32 characters of type alphanumerical + _ + -', $url);

 // command
 $command = '/sbin/zfs create -V '.$size_gib.'g '.$path;
 dangerouscommand($command, $url);
}

function submit_zvol_destroy()
{
 $volname = @$_POST['zvol_name'];
 $command = '/sbin/zfs destroy '.$volname;
 if (@isset($_POST['destroy_zvol']))
  dangerouscommand($command, 'files.php?zvol');
}

?>
