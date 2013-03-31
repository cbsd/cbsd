<?php

function content_pools_create()
{
 global $guru;

 // required libraries
 activate_library('guru');
 activate_library('html');
 activate_library('zfs');

 // call functions
 $zpools = zfs_pool_list();
 $zfsver = guru_zfsversion();

 // process table poollist
 $poollist = array();
 foreach ($zpools as $poolname => $pooldata)
 {
  $activerow = (@$_GET['query'] == $poolname) ? ' class="activerow"' : '';
  $poolspa = zfs_pool_version($poolname);
  $zpool_status = `zpool status $poolname`;
  if (strpos($zpool_status,'raidz3') !== false)
   $redundancy = 'RAID7 (triple parity)';
  elseif (strpos($zpool_status,'raidz2') !== false)
   $redundancy = 'RAID6 (double parity)';
  elseif (strpos($zpool_status,'raidz1') !== false)
   $redundancy = 'RAID5 (single parity)';
  elseif (strpos($zpool_status,'mirror') !== false)
   $redundancy = 'RAID1 (mirroring)';
  else
   $redundancy = 'RAID0 (no redundancy)';
  $poollist[] = array(
   'POOLLIST_ACTIVEROW'		=> $activerow,
   'POOLLIST_POOLNAME'		=> htmlentities(trim($poolname)),
   'POOLLIST_SPA'		=> $poolspa,
   'POOLLIST_REDUNDANCY'	=> $redundancy,
   'POOLLIST_SIZE'		=> $pooldata['size'],
   'POOLLIST_USED'		=> $pooldata['used'],
   'POOLLIST_FREE'		=> $pooldata['free'],
   'POOLLIST_STATUS'		=> $pooldata['status'],
   'POOLLIST_POOLNAME_URLENC'	=> htmlentities(trim($poolname))
  );
 }

 // ZPL list
 $poolzpl = '';
 for ($i = 1; $i <= $zfsver['zpl']; $i++)
  if ($i == $guru['recommended_zfsversion']['zpl'])
   $poolzpl .= '  <option selected="selected" value="'.$i.'">'.$i
    .' (recommended)</option>'.chr(10);
  else
   $poolzpl .= '  <option value="'.$i.'">'.$i.'</option>'.chr(10);

 // SPA list
 $poolspa = '';
 for ($i = 1; $i <= $zfsver['spa']; $i++)
  if ($i == $guru['recommended_zfsversion']['spa'])
   $poolspa .= '  <option selected="selected" value="'.$i.'">'.$i
    .' (recommended)</option>'.chr(10);
  else
   $poolspa .= '  <option value="'.$i.'">'.$i.'</option>'.chr(10);

 // member disks
 $memberdisks = html_memberdisks();

 // new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'Create',
  'PAGE_TITLE'		=> 'Create new pool',
  'POOL_ZPLLIST'	=> $poolzpl,
  'POOL_SPALIST'	=> $poolspa,
  'POOL_MEMBERDISKS'	=> $memberdisks
 );
 return $newtags;
}

function submit_pools_createpool()
{
 // required libraries
 activate_library('guru');
 activate_library('zfs');

 // POST variables
 sanitize(@$_POST['new_zpool_name'], null, $new_zpool, 24);
 $mountpoint = @$_POST['new_zpool_mountpoint'];
 $sectorsize = @$_POST['new_zpool_sectorsize'];
 $url = 'pools.php?create';
 $url2 = 'pools.php?query='.$new_zpool;

 // sanity
 if ($new_zpool != @$_POST['new_zpool_name'])
  friendlyerror('please use only alphanumerical characters for the pool name',
   $url);
 if (strlen($new_zpool) < 1)
  friendlyerror('please enter a name for your new pool', $url);

 // mountpoint
 if ((strlen($mountpoint) > 1) AND ($mountpoint{0} == '/'))
  $options_str = '-m '.$mountpoint.' ';
 else
 {
  // use default mountpoint if not explicitly defined
  $mountpoint = '/' . $new_zpool;
  $options_str = '';
 }

 // filesystem version
 $spa = (int)@$_POST['new_zpool_spa'];
 $zpl = (int)@$_POST['new_zpool_zpl'];
 $sys = guru_zfsversion();
 if (($spa > 0) AND ($spa <= $sys['spa']))
  $options_str .= '-o version='.$spa.' ';
 if (($zpl > 0) AND ($zpl <= $sys['zpl']))
  $options_str .= '-O version='.$zpl.' ';
 $options_str .= '-O atime=off ';

 // extract and format submitted disks to add
 if (is_numeric($sectorsize))
  $vdev = zfs_extractsubmittedvdevs($url, true);
 else
  $vdev = zfs_extractsubmittedvdevs($url, false);
 $redundancy = zfs_extractsubmittedredundancy($_POST['new_zpool_redundancy'],
  $vdev['member_count'], $url);

 // check for member disks
 if ($vdev['member_count'] < 1)
  error('vdev member count zero');

 // warn for RAID0 with more than 1 disk (could be a mistake)
 if (($vdev['member_count'] > 1) AND ($redundancy == ''))
  page_feedback('you selected RAID0 with more than one disk; are you sure '
   .'that is what you wanted?', 'a_warning');

 // array with commands to execute
 $commands = array();

 // handle sector size overrides
 // we do this by creating GNOP providers which override the sector size
 // this will force ashift to be different (inspect using zdb)
 // this also works across reboots, and the .nop providers are only needed once
 if (is_numeric($sectorsize))
  if (is_array($vdev['member_disks']))
   foreach ($vdev['member_disks'] as $vdevdisk)
    $commands[] = '/sbin/gnop create -S '.(int)$sectorsize.' /dev/'.$vdevdisk;

 // TODO: SECURITY
 // assemble zpool create command
 $commands[] = '/sbin/zpool create '.$options_str.$new_zpool.' '.$redundancy.' '
  .$vdev['member_str'];
 $commands[] = '/usr/sbin/chown -R nfs:nfs '.$mountpoint;

 // defer to dangerouscommand function
 dangerouscommand($commands, $url2);
}

?>
