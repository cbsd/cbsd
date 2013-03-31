<?php

function content_disks_query()
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

 // list only the queried disk
 $disks = @array($querydisk => $disks[$querydisk]);
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
 return @array(
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

?>
