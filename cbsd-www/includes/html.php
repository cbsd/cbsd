<?php

function html_zfspools($zfs_pool_list = false)
{
 // fetch pool list unless supplied as argument
 if (!is_array($zfs_pool_list))
 {
  activate_library('zfs');
  $zfs_pool_list = zfs_pool_list();
 }
 // craft string containing all selectbox options
 $options = '';
 foreach ($zfs_pool_list as $poolname => $pooldata)
  $options .= '<option value="'.htmlentities($poolname).'">'
   .htmlentities($poolname).'</option>'.chr(10);
 return $options;
}

function html_zfsfilesystems($zfs_filesystem_list = false)
{
 // fetch filesystem list unless supplied as argument
 if (!is_array($zfs_filesystem_list))
 {
  activate_library('zfs');
  $zfs_filesystem_list = zfs_filesystem_list();
 }
 // craft string containing all selectbox options
 $options = '';
 foreach ($zfs_filesystem_list as $fsname => $fsdata)
  $options .= '<option value="'.htmlentities($fsname).'">'
   .htmlentities($fsname).'</option>'.chr(10);
 return $options;
}

function html_memberdisks($physdisks = false, $only_gpt = false)
{
 // required libraries
 activate_library('disk');
 activate_library('zfs');

 // call functions
 if (!is_array($physdisks))
  $physdisks = disk_detect_physical();
 $gpart = disk_detect_gpart();
 $labels = disk_detect_label();
 $members = zfs_pool_memberdisks();

 // craft member disk string
 $mdstring = '';
 foreach ($physdisks as $disk => $diskdata)
 {
  // check for GPT label
  $realdisk = false;
  if (@strlen($gpart[$disk]['label']) > 0)
   $realdisk = 'gpt/'.$gpart[$disk]['label'];
  elseif ((@strlen($labels[$disk]) > 0) AND ($only_gpt == false))
   $realdisk = 'label/'.$labels[$disk];
  // determine size in human size
  $size_human = sizehuman((int)$diskdata['mediasize'], 1);
  // skip disks which have no GPT or GEOM label
  if (!$realdisk)
   continue;
  if ($poolname = zfs_pool_ismemberdisk($realdisk, $members))
   $mdstring .= '<input type="checkbox" name="addmember_'.htmlentities($realdisk).'" '
    .'disabled="disabled" />'.chr(10).'<span class="diskdisabled">'
    .'disk <b>'.htmlentities($disk).'</b>, identified with label '
    .'<b>'.htmlentities($realdisk).'</b></span> ('.$size_human.')'
    .'<span class="diskinuse">in use as member disk for ZFS pool '
    .'<b>'.htmlentities($poolname).'</b></span><br />'.chr(10);
  else
   $mdstring .= '<input type="checkbox" name="addmember_'.htmlentities($realdisk).'" '
    .'/> disk <b>'.htmlentities($disk).'</b>, identified with label '
    .'<b>'.htmlentities($realdisk).'</b> ('.$size_human.')<br />'.chr(10);
 }
 return $mdstring;
}

function html_wholedisks($physdisks = false)
{
 // required libraries
 activate_library('disk');

 // call functions
 if (!is_array($physdisks))
  $physdisks = disk_detect_physical();
 $gpart = disk_detect_gpart();
 $labels = disk_detect_label();
 $members = zfs_pool_memberdisks();

 // craft member disk string
 $htmlstring = '';
 foreach ($physdisks as $disk => $diskdata)
 {
  // check for GPT label
  $label = false;
  if (@strlen($gpart[$disk]['label']) > 0)
   $label = 'gpt/'.$gpart[$disk]['label'];
  elseif ((@strlen($labels[$disk]) > 0) AND ($only_gpt == false))
   $label = 'label/'.$labels[$disk];
  // determine size in human size
  $size_human = sizehuman((int)$diskdata['mediasize'], 1);
  // set labelname
  $labelname = '';
  if ($label)
  {
   $labelname = ', identified with label <b>'.htmlentities($label).'</b>';
   $membercheckname = $label;
  }
  else
   $membercheckname = $disk;
  if ($poolname = zfs_pool_ismemberdisk($membercheckname, $members))
   $htmlstring .= 
    '<input type="checkbox" name="addwholedisk_'.htmlentities($disk).'" '
    .'disabled="disabled" />'.chr(10).'<span class="diskdisabled">'
    .'disk <b>'.htmlentities($disk).'</b>'.$labelname.'</span> ('.$size_human.')'
    .'<span class="diskinuse">in use as member disk for ZFS pool '
    .'<b>'.htmlentities($poolname).'</b></span><br />'.chr(10);
  else
   $htmlstring .= 
    '<input type="checkbox" name="addwholedisk_'.htmlentities($disk).'" '
    .'/> disk <b>'.htmlentities($disk).'</b>'.$labelname.' ('.$size_human
    .')<br />'.chr(10);
 }
 return $htmlstring;
}

?>
