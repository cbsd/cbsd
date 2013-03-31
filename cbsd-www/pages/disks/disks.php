<?php

function content_disks_disks()
{
 // required library
 activate_library('disk');

 // call functions
 $disks = disk_detect_physical();
 $dmesg = disk_detect_dmesg();
 $gpart = disk_detect_gpart();
 $labels = disk_detect_label();
 $gnop = disk_detect_gnop();

 // variables
 $diskcount = @(int)count($disks);
 $querydisk = @$_GET['query'];

 // list each disk (partition)
 $physdisks = array();
 if (@is_array($disks))
  foreach ($disks as $diskname => $data)
  {
   $activerow = ($querydisk == $diskname) ? 'class="activerow"' : '';
   // acquire GNOP sector size (for sectorsize override)
   $gnop_sect = (int)@$gnop['label/'.$labels[$diskname]]['sectorsize'];
   if ($gnop_sect < 512)
    $gnop_sect = (int)@$gnop['gpt/'.$gpart[$diskname]['label']]['sectorsize'];
   if (@$gnop_sect > 0)
   {
    // GNOP is active
    $sectorsize = @sizebinary($gnop_sect);
    $sectorclass = 'high';
   }
   elseif ($data['sectorsize'] == '512')
   {
    // standard sector size
    $sectorsize = '512 B';
    $sectorclass = 'network_sector_normal';
   }
   else
   {
    // native high sector size
    $sectorsize = @sizebinary($data['sectorsize']);
    $sectorclass = 'high';
   }

   // process GPT/GEOM label string
   $labelstr = '';
   if (@strlen($labels[$diskname]) > 0)
    $labelstr .= 'GEOM: '.@htmlentities($labels[$diskname]);
   if (@strlen($gpart[$diskname]['label']) > 0)
   {
    if (strlen($labelstr) > 0)
     $labelstr .= '<br />';
    $labelstr .= 'GPT: '.@htmlentities($gpart[$diskname]['label']);
   }

   // add new row to table array
   $physdisks[] = array(
    'DISK_ACTIVEROW'		=> $activerow,
    'DISK_NAME'			=> htmlentities($diskname),
    'DISK_LABEL'		=> $labelstr,
    'DISK_SIZE_LEGACY'		=> @sizehuman($data['mediasize'], 1),
    'DISK_SIZE_BINARY'		=> @sizebinary($data['mediasize'], 1),
    'DISK_CLASS_SECTOR'		=> $sectorclass,
    'DISK_SIZE_SECTOR'		=> $sectorsize,
    'DISK_IDENTIFY'		=> @htmlentities($dmesg[$diskname])
   );
  }

 // process queried disk (for format box)
 if ($querydisk)
 {
  $formatclass = 'normal';
  if (@strlen($gpart[$querydisk]['label']) > 0)
  {
   $gptchecked = 'checked="checked"';
   $gptlabel = htmlentities($gpart[$querydisk]['label']);
   $geomchecked = '';
   $geomlabel = '';
  }
  elseif (@strlen($labels[$querydisk]) > 0)
  {
   $gptchecked = '';
   $gptlabel = '';
   $geomchecked = 'checked="checked"';
   $geomlabel = htmlentities($labels[$querydisk]);
  }
 }
 else
 {
  $formatclass = 'hidden';
 }

 // export new tags
 return array(
  'PAGE_ACTIVETAB'		=> 'Physical disks',
  'PAGE_TITLE'			=> 'Physical disks',
  'TABLE_DISKS_PHYSDISKS'	=> $physdisks,
  'DISKS_DISKCOUNT'		=> $diskcount,
  'QUERY_DISKNAME'		=> $querydisk,
  'FORMAT_CLASS'		=> $formatclass,
  'FORMAT_GPTCHECKED'		=> @$gptchecked,
  'FORMAT_GEOMCHECKED'		=> @$geomchecked,
  'FORMAT_GPTLABEL'		=> @$gptlabel,
  'FORMAT_GEOMLABEL'		=> @$geomlabel
 );
}

function submit_disks_formatdisk()
{
 global $guru;

 // required libraries
 activate_library('disk');
 activate_library('super');
 activate_library('zfs');

 // variables
 $url = 'disks.php';

 // sanity on disk device
 sanitize(@$_POST['formatdisk_diskname'], null, $disk);
 $disk_dev = '/dev/'.$disk;
 if (!file_exists($disk_dev))
  friendlyerror('Invalid disk: "'.$disk.'"; does not exist!', $url);

 // redirection url
 $url2 = 'disks.php?query='.$disk;

 // sanity on label name
 $san_geom = sanitize(@$_POST['geom_label'], null, $geom_label, 16);
 $san_gpt = sanitize(@$_POST['gpt_label'], null, $gpt_label, 16);
 if (@$_POST['format_type'] == 'geom')
 {
  $disklabel = 'label/'.$geom_label;
  if (!$san_geom)
   friendlyerror('please enter a valid GEOM label name for your disk '
    .'(alphanumerical + underscore + dash characters allowed', $url2);
 }
 elseif (@$_POST['format_type'] == 'gpt')
 {
  $disklabel = 'gpt/'.$gpt_label;
  if (!$san_gpt)
   friendlyerror('please enter a valid GPT label name for your disk '
    .'(alphanumerical + underscore + dash characters allowed', $url2);
 }
 else
  friendlyerror('please select a partition schedule, GPT or GEOM.', $url2);

 // check whether disk is part of a pool
 $labels = disk_detect_label();
 $gpart = disk_detect_gpart();
 if (@isset($labels[$disk]))
  $labelname = 'label/'.$labels[$disk];
 elseif (@isset($gpart[$disk]['label']))
  $labelname = 'gpt/'.$gpart[$disk]['label'];
 else
  $labelname = false;
 $memberdisks = zfs_pool_memberdisks();
 if ($labelname != false)
  $poolname = zfs_pool_ismemberdisk($labelname, $memberdisks, false);
 else
  $poolname = zfs_pool_ismemberdisk($disk, $memberdisks, false);
 if ($poolname != false)
  friendlyerror('disk <b>'.$disk.'</b> is a member of pool <b>'.$poolname
   .'</b> and cannot be formatted! Destroy the pool first.', $url2);

 // random write
 if (@$_POST['random_write'] == 'on')
 {
  $result = super_script('random_write', $disk);
  if ($result['rv'] != 0 AND $result['rv'] != 1)
   error('Random writing disk '.$disk.' failed, got return value '
    .(int)$result['rv'].'. Command output:<br /><br />'
    .nl2br($result['output_str']));
 }
 // zero-write
 if (@$_POST['zero_write'] == 'on')
 {
  $result = super_script('zero_write', $disk);
  if ($result['rv'] != 0 AND $result['rv'] != 1)
   error('Zero writing disk '.$disk.' failed, got return value '
    .(int)$result['rv'].'. Command output:<br /><br />'
    .nl2br($result['output_str']));
 }
 // secure erase
 if (@$_POST['secure_erase'] == 'on')
 {
  $result = super_script('secure_erase', $disk);
  if ($result['rv'] != 0 AND $result['rv'] != 1)
   error('Secure Erasing disk '.$disk.' failed, got return value '
    .(int)$result['rv'].'. Command output:<br /><br />'.$result['output_str']);
 }

 // format disk; cleaning any existing partitions
 $result = super_script('format_disk', $disk);
 if ($result['rv'] != 0)
  error('Formatting disk '.$disk.' failed, got return value '
   .(int)$result['rv'].'. Command output:<br /><br />'.$result['output_str']);

 // destroy existing GEOM label
 super_script('geom_label_destroy', $disk);

 // abort if the device exists -- this check has to happen AFTER initial format
 usleep(50000);
 if (file_exists('/dev/'.$disklabel))
  friendlyerror('you already have a disk with the label <b>'.$disklabel
   .'</b>, please choose another name!', $url2);

 // GEOM formatting
 if (@$_POST['format_type'] == 'geom')
 {
  // create new GEOM label
  super_script('geom_label_create', $disk.' '.$geom_label);
 }

 // GPT formatting
 if (@$_POST['format_type'] == 'gpt')
 {
  // gather diskinfo
  $diskinfo = disk_info($disk);

  // reservespace is the space we leave unused at the end of GPT partition
  $reservespace = @$_POST['gpt_reservespace'];
  $reservespace = ((!is_numeric($reservespace)) OR (int)$reservespace < 0) ?
   1 : (int)$reservespace;
  // TODO: this assumes sector size = 512 bytes!
  $reserve_sect = $reservespace * (1024 * 2);
  // total sector size

  // determine size of data partition ($data_size)
  // $data_size = sectorcount minus reserve sectors + 33 for gpt + 2048 offset
  $data_size = $diskinfo['sectorcount'] - ($reserve_sect + 33 + 2048);
  // round $data_size down to multiple of 1MiB or 2048 sectors
  $data_size = floor($data_size / 2048) * 2048;
  // minimum 64MiB (assuming 512-byte sectors)
  if ((int)$data_size < (64 * 1024 * 2))
   error('The data partition needs to be at least 64MiB large; '
    .'try reserving less space');

  // bootcode (use from webinterface files directory unless not present)
  $fd = $guru['docroot'].'/files/';
  if (file_exists($fd.'pmbr'))
   $pmbr = $fd.'pmbr';
  else
  {
   $pmbr = '/boot/pmbr';
   page_feedback('could not use <b>pmbr</b> from webinterface - '
    .'using system image version', 'c_notice');
  }
  if (file_exists($fd.'gptzfsboot'))
   $gptzfsboot = $fd.'gptzfsboot';
  else
  {
   $gptzfsboot = '/boot/gptzfsboot';
   page_feedback('could not use <b>gptzfsboot</b> from webinterface'
    .' - using system image version', 'c_notice');
  }

  // create GPT partition scheme
  super_script('create_gpt_partitions', $disk.' "'.$gpt_label.'" '
   .(int)$data_size.' '.$pmbr.' '.$gptzfsboot);
 }

 // microsleep
 usleep(50);

 // redirect
 $label = ($_POST['format_type'] == 'geom') ? $geom_label : $gpt_label;
 friendlynotice('disk <b>'.htmlentities($disk).'</b> has been formatted with '
  .'<b>'.@strtoupper($_POST['format_type']).'</b>, and will be identified by '
  .'the label <b>'.@htmlentities($label).'</b>', $url);
 die();
}

?>
