<?php

function content_pools_query()
{
 // required libraries
 activate_library('zfs');

 // call functions
 $zpools = zfs_pool_list();

 // pool count
 $poolcount = count($zpools);
 $poolcountstr = ($poolcount == 1) ? '' : 's';

 // process table poollist
 $poollist = array();
 foreach ($zpools as $poolname => $pooldata)
 {
  $class = (@$_GET['query'] == $poolname) ? 'activerow' : 'normal';
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
  $statusclass = 'normal';
  if ($pooldata['status'] == 'ONLINE')
   $statusclass = 'pool_online';
  elseif ($pooldata['status'] == 'FAULTED')
  {
   $statusclass = 'pool_faulted';
   if ($class == 'normal')
    $class = 'pool_faulted';
  }
  elseif ($pooldata['status'] == 'DEGRADED')
  {
   $statusclass = 'pool_degraded';
   if ($class == 'normal')
    $class = 'pool_degraded';
  }
  $poollist[] = array(
   'POOLLIST_CLASS'		=> $class,
   'POOLLIST_POOLNAME'		=> htmlentities(trim($poolname)),
   'POOLLIST_SPA'		=> $poolspa,
   'POOLLIST_REDUNDANCY'	=> $redundancy,
   'POOLLIST_SIZE'		=> $pooldata['size'],
   'POOLLIST_USED'		=> $pooldata['used'],
   'POOLLIST_FREE'		=> $pooldata['free'],
   'POOLLIST_STATUS'		=> $pooldata['status'],
   'POOLLIST_STATUSCLASS'	=> $statusclass,
   'POOLLIST_POOLNAME_URLENC'	=> htmlentities(trim($poolname))
  );
 }

 // querypool
 $querypool = @trim($_GET['query']);
 $pool_esc = @htmlentities($querypool);

 // call functions
 $scrub = zfs_pool_isbeingscrubbed($querypool);
 $pool = zfs_pool_status($querypool);
 $ashift = zfs_pool_ashift($querypool);

 // make some data bold by adding HTML tags
 $regexp = array(
  '/([0-9]{1,4}(\.[0-9]{1,2})?[KMGT]\/s),/',
  '/ ([0-9]+\.[0-9]+%) done/'
 );
 $repl = array(
  '<b>$1</b>,',
  ' <b>$1</b> done'
 );
 $pool['scrub'] = preg_replace($regexp, $repl, $pool['scrub']);

 // suppress warning about upgrading pool
 $suppresstext = 'The pool is formatted using an older on-disk format.';
 if (substr(@$pool['status'], 0, strlen($suppresstext)) == $suppresstext)
 {
  $pool['status'] = '';
  $pool['action'] = '';
 }

 // pool details
 $pooldetails = array();
 $detailvars = array(
  'Status'	=> 'state',
  'Description'	=> 'status',
  'Action'	=> 'action',
  'See'		=> 'see',
  'Scrub'	=> 'scrub',
  'Config'	=> 'config'
 );
 $statusclass = 'normal';
 if (@$pool['state'] == 'ONLINE')
  $statusclass = 'status_online';
 elseif (@$pool['state'] == 'FAULTED')
  $statusclass = 'status_faulted';
 elseif (@$pool['state'] == 'UNAVAIL')
  $statusclass = 'status_faulted';
 elseif (@$pool['state'] == 'DEGRADED')
  $statusclass = 'status_degraded';
 foreach ($detailvars as $name => $var)
  if (strlen(@$pool[$var]) > 0)
   $pooldetails[] = array(
    'POOLDETAILS_CLASS'	=> ($var == 'state') ? $statusclass : 'normal',
    'POOLDETAILS_NAME'	=> $name,
    'POOLDETAILS_VALUE'	=> @nl2br($pool[$var])
   );

 // ashift value
 if ((int)$ashift > 0)
 {
  $ashift_sectorsize = pow(2, $ashift);
  $pooldetails[] = array(
   'POOLDETAILS_NAME'	=> 'Ashift',
   'POOLDETAILS_VALUE'	=> 'pool is optimized for '.sizebinary($ashift_sectorsize)
    .' sector disks (ashift='.$ashift.')'
  );
 }

 // scrub status
 if ($scrub)
 {
  $scrubname = 'Stop';
  $scrubaction = 'stop';
 }
 else
 {
  $scrubname = 'Start';
  $scrubaction = 'start';
 }

 // process members table
 $memberdisks = array();
 if (is_array($pool['members']))
  foreach ($pool['members'] as $member)
  {
   $memberdisks[] = array(
    'MEMBER_NAME'	=> @htmlentities($member['name']),
    'MEMBER_STATE'	=> @htmlentities($member['state']),
    'MEMBER_READ'	=> @htmlentities($member['read']),
    'MEMBER_WRITE'	=> @htmlentities($member['write']),
    'MEMBER_CHECKSUM'	=> @htmlentities($member['cksum']),
   );
  }

 // new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'Pool status',
  'PAGE_TITLE'		=> 'Pool '.$querypool,
  'TABLE_POOL_LIST'	=> $poollist,
  'TABLE_POOL_DETAILS'	=> $pooldetails,
  'TABLE_MEMBERDISKS'	=> $memberdisks,
  'POOL_COUNT'          => $poolcount,
  'POOL_COUNT_STRING'   => $poolcountstr,
  'QUERY_POOLNAME'	=> $pool_esc,
  'QUERY_POOLSTATUS'	=> @htmlentities($pool['state']),
  'QUERY_DESCRIPTION'	=> @htmlentities($pool['status']),
  'QUERY_ACTION'	=> @htmlentities($pool['action']),
  'QUERY_SEE'		=> @htmlentities($pool['see']),
  'QUERY_SCRUB'		=> @htmlentities($pool['scrub']),
  'QUERY_CONFIG'	=> @htmlentities($pool['config']),
  'QUERY_SCRUBACTION'	=> $scrubaction,
  'QUERY_SCRUBNAME'	=> $scrubname,
  'QUERY_ASHIFT'	=> $ashift
 );
 return $newtags;
}

function submit_pools_scrub()
{
 // pool we are working on
 $poolname = @$_POST['poolname'];
 $url = 'pools.php?query='.$poolname;

 // required libary
 activate_library('zfs');

 // check whether to start or stop a running scrub
 if (@$_POST['pool_startscrub'])
  zfs_pool_scrub($poolname);
 elseif (@$_POST['pool_stopscrub'])
  zfs_pool_scrub($poolname, true);

 // redirect back to pool query page
 redirect_url($url);
}

function submit_pools_operations()
{
 // variables
 $url1 = 'pools.php';
 sanitize(@$_POST['poolname'], null, $poolname);
 $url2 = 'pools.php?query='.$poolname;

 // pool operations - handle with dangerouscommand function
 if (@isset($_POST['upgrade_pool']))
  dangerouscommand('/sbin/zpool upgrade '.$poolname, $url2);
 elseif (@isset($_POST['export_pool']))
  dangerouscommand('/sbin/zpool export '.$poolname, $url1);
 elseif (@isset($_POST['destroy_pool']))
 {
  // required libraries
  activate_library('samba');
  activate_library('zfs');

  // get list of filesystems (may return false if pool is faulted)
  $fslist = zfs_filesystem_list($poolname, '-r -t filesystem');
  if ($fslist == false)
  {
   $fslist = array();
   page_feedback('this pool does not have any active filesystems - '
    .'unable to scan for active Samba shares!', 'a_warning');
  }

  // remove any samba filesystem
  $sharesremoved = 0;
  foreach ($fslist as $fsname => $fsdata)
   if (strlen(@$fsdata['mountpoint']) > 1)
    $sharesremoved += (int)samba_removesharepath($fsdata['mountpoint']);

  // display message if applicable
  if ($sharesremoved > 1)
   page_feedback('removed <b>'.(int)$sharesremoved.' Samba shares</b> that were'
    .' attached to the filesystems you are about to destroy', 'c_notice');
  elseif ($sharesremoved == 1)
   page_feedback('removed <b>one Samba share</b> that was attached to one of '
    .'the filesystems you are about to destroy', 'c_notice');

  // start command array
  $command = array();

  // scan for swap volumes and deactivate them prior to destroying them
  $vollist = zfs_filesystem_list($poolname, '-r -t volume');
  exec('/sbin/swapctl -l', $swapctl_raw);
  $swapctl = @implode(chr(10), $swapctl_raw);
  if (@is_array($vollist))
   foreach ($vollist as $volname => $voldata)
   {
    $fsdetails = zfs_filesystem_getproperties($volname, 'org.freebsd:swap');
    // check if volume is in use as a SWAP device
    if (@$fsdetails['org.freebsd:swap'][2] == 'on')
     if (@strpos($swapctl, '/dev/zvol/'.$volname) !== false)
      $command[] = '/sbin/swapoff /dev/zvol/'.$volname;
   }
  // display message if swap volumes detected
  if (count($command) == 1)
   page_feedback('if you continue, one SWAP volume will be deactivated',
    'c_notice');
  if (count($command) > 1)
   page_feedback('if you continue, '.count($command).' SWAP volumes will be '
    .'deactivated', 'c_notice');

  // destroy pool command
  $command[] = '/sbin/zpool destroy '.$poolname;

  // defer to dangerous command function
  dangerouscommand($command, $url1);
 }
 friendlyerror('no operation detected', $url2);
}

?>
