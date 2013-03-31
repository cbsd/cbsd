<?php

function content_pools_expansion()
{
 // required libraries
 activate_library('html');
 activate_library('zfs');

 // call functions
 $zpools = zfs_pool_list();

 // pool selectbox
 $poolselectbox = html_zfspools($zpools);

 // member disks
 $memberdisks = html_memberdisks();

 // expand disable
 $expanddisable = (@empty($zpools)) ? 'disabled="disabled"' : '';

 // new tags
 $newtags = array(
  'PAGE_ACTIVETAB'		=> 'Expansion',
  'PAGE_TITLE'			=> 'Expansion',
  'POOL_SELECTBOX'		=> $poolselectbox,
  'POOL_MEMBERDISKS'		=> $memberdisks,
  'POOL_EXPANDDISABLE'		=> $expanddisable
 );
 return $newtags;
}

function submit_pools_expandpool()
{
 // required library
 activate_library('zfs');

 // gather data
 $zpool_name = @$_POST['exp_zpool_name'];
 $url = 'pools.php?expansion';
 $url2 = 'pools.php?query='.$zpool_name;

 // call functions
 $status = zfs_pool_status($zpool_name);
 $vdevs = zfs_extractsubmittedvdevs($url);

 // sanity checks
 if (strlen($zpool_name) < 0)
  friendlyerror('invalid pool name', $url);
 if (@$status['pool'] != $zpool_name)
  friendlyerror('this pool is unknown to the system', $url);
 if ((@$status['state'] != 'ONLINE') AND 
     (@$status['state'] != 'DEGRADED'))
  friendlyerror('this pool is not healthy (<b>'
   .@$status['state'].'</b> instead of ONLINE or DEGRADED)', $url);
 if (@$vdevs['member_count'] < 1)
  friendlyerror('no member disks selected', $url);
 $redundancy = zfs_extractsubmittedredundancy(@$_POST['exp_zpool_redundancy'],
  $vdevs['member_count'], $url);

 // warn if user chose RAID0 while pool has redundancy (mirror/raidz1/2/3)
 if ($redundancy == '')
 {
  $raid0 = true;
  foreach ($status['members'] as $data)
   if (strpos($data['name'], 'mirror') !== false)
    $raid0 = false;
   elseif (strpos($data['name'], 'raidz') !== false)
    $raid0 = false;
  if (!$raid0)
   page_feedback('you are adding a RAID0 vdev to a pool with redundancy, '
    .'are you sure that is what you want?', 'a_warning');
 }

 // mixed redundancy is mandatory for non-standard expansion
 if (@isset($_POST['exp_mixed_redundancy']))
  $mixed_redundancy = '-f ';
 else
  $mixed_redundancy = '';

 // defer to dangerouscommand function
 $command = '/sbin/zpool add '.$mixed_redundancy.$zpool_name.' '
  .$redundancy.' '.$vdevs['member_str'];
 dangerouscommand($command, $url2);
}

?>
