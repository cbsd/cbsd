<?php

function content_system_install_step4()
{
 // required library
 activate_library('guru');

 // variables
 $dist = trim($_GET['dist']);
 $sysver = trim($_GET['sysver']);
 $target = trim($_GET['target']);

 // call functions
 $sysvers = guru_fetch_systemversions();

 // process system version name
 if (substr($sysver, 0, strlen('HASH')) == 'HASH')
 {
  // unknown system version from CD/USB media; treat differently
  $sysver = trim(substr($sysver, strlen('HASH')));
  $sysver_str = $sysver.' (unknown by central database!)';
 }
 else
  $sysver_str = $sysver;

 // redirect back to step 2 if no system version found
 if (strlen($sysver) < 1)
  redirect_url('system.php?install&dist='.$dist);

 // system location
 if (@isset($sysvers[$sysver]['md5hash']))
  $sysloc = guru_locate_systemimage($sysvers[$sysver]['md5hash']);
 elseif (strlen($sysver) == 32)
  $sysloc = guru_locate_systemimage($sysver);
 if (!$sysloc)
 {
  page_feedback('system version <b>'.$sysver.'</b> not found!', 'a_error');
  $sysloc = '!!NOT FOUND!!';
 }

 // system size
 $syssize = @(int)filesize($sysloc);
 $syssize_binary = sizebinary($syssize, 1);
 if (!@isset($sysvers[$sysver]))
  $syssize_binary .= ' (cannot verify)';
 else
 {
  if (@$sysvers[$sysver]['filesize'] != $syssize)
   $syssize_binary = '<b>Invalid size:</b> '.$syssize.' bytes '
    .'(expected: '.(int)$sysvers[$sysver]['filesize'].' bytes)';
  else
   $syssize_binary .= ' ('.$syssize.' bytes)';
 }
 $compressfactor = 3;
 $compressfactor_txt = '3.0';
 $syssize_uncompressed = $syssize * $compressfactor;
 $syssize_uncompressed_binary = sizebinary($syssize_uncompressed, 1);

 // checksum
 $md5 = @isset($sysvers[$sysver]) 
  ? @$sysvers[$sysver]['md5hash'] : $sysver.' (cannot verify)';
 $sha1 = (@isset($sysvers[$sysver]))
  ? @$sysvers[$sysver]['sha1hash'] : '<b>unknown</b> (cannot verify)';

 // Root-on-ZFS distribution
 if ($dist == 'rootonzfs')
 {
  // required library
  activate_library('zfs');
  // call functions
  $zfslist = zfs_filesystem_list($target, '-r');

  // target filesystem
  $targetfs = substr($sysver, 0, 10);
  $targetprefix = $target.'/zfsguru/';
  $class_targetinuse = 'hidden';
  if (@isset($zfslist[$targetprefix.$targetfs]))
   $class_targetinuse = 'normal';

  // target free space
  $targetfreespace = @(int)disk_free_space('/'.$target);
  $targetfreespace_afterinstall = $targetfreespace - $syssize_uncompressed;
  $targetfreespace_after_binary = sizebinary($targetfreespace_afterinstall, 1);
  $targetfreespace_binary = sizebinary($targetfreespace, 1);

  // swap size table
  $table_roz_swapsize = array();
  $swap_availspace = $syssize - $syssize_uncompressed;
  $swaplist = array(
   '0.125'	=> '128 MiB (minimum)</option>',
   '0.25'	=> '256 MiB',
   '0.5'	=> '512 MiB',
   '1.0'	=> '1 GiB',
   '2.0'	=> '2 GiB',
   '4.0'	=> '4 GiB',
   '6.0'	=> '6 GiB',
   '8.0'	=> '8 GiB',
   '10.0'	=> '10 GiB',
   '16.0'	=> '16 GiB',
   '20.0'	=> '20 GiB',
   '32.0'	=> '32 GiB'
  );
  foreach ($swaplist as $value => $name)
  {
   if (($value * 1024 * 1024 * 1024) > $targetfreespace_afterinstall)
    continue;
   $swapname = $name;
   $selected = '';
   if ($value == '2.0')
   {
    $swapname = $name.' (default)';
    $selected = 'selected="selected"';
   }
   $table_roz_swapsize[] = array(
    'SWAP_NAME'		=> $swapname,
    'SWAP_VALUE'	=> $value,
    'SWAP_SELECTED'	=> $selected
   );
  }
  if (count($table_roz_swapsize) > 1)
  {
   $arr = $table_roz_swapsize;
   $lastkey = array_pop(array_keys($arr));
   $table_roz_swapsize[$lastkey]['SWAP_NAME'] .= ' (maximum)';
   if ((double)$table_roz_swapsize[$lastkey]['SWAP_VALUE'] < 2.0)
    $table_roz_swapsize[$lastkey]['SWAP_SELECTED'] = 'selected="selected"';
  } 
  
  // set distribution box
  $class_roz = 'normal';
  $class_emb = 'hidden';
 }

 // embedded distribution
 if ($dist == 'embedded')
 {
  // set distribution box
  $class_roz = 'hidden';
  $class_emb = 'normal';
 }

 // export new tags
 $newtags = @array(
  'PAGE_ACTIVETAB'	=> 'Install',
  'PAGE_TITLE'		=> 'Install (step 4)',
  'TABLE_ROZ_SWAPSIZE'	=> $table_roz_swapsize,
  'CLASS_EMB'		=> $class_emb,
  'CLASS_ROZ'		=> $class_roz,
  'CLASS_TARGETINUSE'	=> $class_targetinuse,
  'INSTALL_DIST'	=> $dist,
  'INSTALL_SYSVER'	=> $sysver,
  'INSTALL_SYSVER_STR'	=> $sysver_str,
  'INSTALL_TARGET'	=> $target,
  'ROZ_TARGETPREFIX'	=> $targetprefix,
  'ROZ_TARGETFS'	=> $targetfs,
  'INSTALL_SYSLOC'	=> $sysloc,
  'INSTALL_SYSSIZE'	=> $syssize_binary,
  'INSTALL_SYSSIZE_UNC'	=> $syssize_uncompressed_binary,
  'INSTALL_CFACTOR'	=> $compressfactor_txt,
  'INSTALL_SYSMD5'	=> $md5,
  'INSTALL_SYSSHA1'	=> $sha1,
  'INSTALL_FREESPACE'	=> $targetfreespace_binary,
  'INSTALL_FREESPACE_A'	=> $targetfreespace_after_binary
 );
 return $newtags;
}

?>
