<?php

function content_internal_welcome()
{
 global $guru;

 // classes
 $class_w0 = 'hidden';
 $class_w1 = 'hidden';
 $class_w2 = 'hidden';
 $class_w3 = 'hidden';
 $class_w4 = 'hidden';
 $class_w5 = 'hidden';
 if (@isset($_GET['welcome5']))
  $class_w5 = 'normal';
 elseif (@isset($_GET['welcome4']))
  $class_w4 = 'normal';
 elseif (@isset($_GET['welcome3']))
  $class_w3 = 'normal';
 elseif (@isset($_GET['welcome2']))
  $class_w2 = 'normal';
 elseif (@isset($_GET['welcome1']))
  $class_w1 = 'normal';
 else
 {
  $class_w0 = 'normal';
  page_rawfile('pages/internal/intro.page');
  die();
 }

 // step 1: protection
 if ($class_w1 == 'normal')
 {
  $w1_ac_1 = (@$_SESSION['welcomewizard']['access_control'] == 1) ? 
   'checked="checked"' : '';
  $w1_ac_2 = ((@$_SESSION['welcomewizard']['access_control'] == 2) OR
              (!@isset($_SESSION['welcomewizard']['access_control']))) ? 
   'checked="checked"' : '';
  $w1_ac_3 = (@$_SESSION['welcomewizard']['access_control'] == 3) ? 
   'checked="checked"' : '';
  $w1_noauth = (@strlen($_SESSION['welcomewizard']['authentication']) > 0) ?
   '' : 'checked="checked"';
  $w1_auth = (@strlen($_SESSION['welcomewizard']['authentication']) > 0) ?
   'checked="checked"' : '';
  $w1_auth_pw = (@strlen($_SESSION['welcomewizard']['authentication']) > 0) ?
   $_SESSION['welcomewizard']['authentication'] : '';
  $w1_user_ipaddr = $_SERVER['REMOTE_ADDR'];
 }

 // step 2: physical disks
 $physdisks = array();
 if ($class_w2 == 'normal')
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
     'DISK_ACTIVEROW'            => $activerow,
     'DISK_NAME'                 => htmlentities($diskname),
     'DISK_LABEL'                => $labelstr,
     'DISK_SIZE_LEGACY'          => @sizehuman($data['mediasize'], 1),
     'DISK_SIZE_BINARY'          => @sizebinary($data['mediasize'], 1),
     'DISK_CLASS_SECTOR'         => $sectorclass,
     'DISK_SIZE_SECTOR'          => $sectorsize,
     'DISK_IDENTIFY'             => @htmlentities($dmesg[$diskname])
    );
   }
 }

 // step 3: ZFS pools
 $poollist = array();
 $import_buttons = '';
 $import_rawoutput = '';
 if ($class_w3 == 'normal')
 {
  // required libraries
  activate_library('html');
  activate_library('guru');
  activate_library('zfs');

  // call functions
  $zpools = zfs_pool_list();
  $zfsver = guru_zfsversion();

  // pool count
  $poolcount = count($zpools);
  $poolcountstr = ($poolcount == 1) ? '' : 's';

  // process table poollist
  $bootablepools = false;
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
    'POOLLIST_ACTIVEROW'         => $activerow,
    'POOLLIST_POOLNAME'          => htmlentities(trim($poolname)),
    'POOLLIST_SPA'               => $poolspa,
    'POOLLIST_REDUNDANCY'        => $redundancy,
    'POOLLIST_SIZE'              => $pooldata['size'],
    'POOLLIST_USED'              => $pooldata['used'],
    'POOLLIST_FREE'              => $pooldata['free'],
    'POOLLIST_STATUS'            => $pooldata['status'],
    'POOLLIST_POOLNAME_URLENC'   => htmlentities(trim($poolname))
   );

   // check boot status by looking whether at least one member has GPT label
   $poolstatus = zfs_pool_status($poolname);
   foreach ($poolstatus['members'] as $memberdata)
    if (@substr($memberdata['name'], 0, strlen('gpt/')) == 'gpt/')
     $bootablepools = true;
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

  // whole disks
  $wholedisks = html_wholedisks();

  // import pool buttons
  if (@!isset($_SESSION['welcomewizard']['noimportablepools']))
   $_SESSION['welcomewizard']['noimportablepools'] = false;
  $noimportablepools = $_SESSION['welcomewizard']['noimportablepools'];
  $scannedforpools = false;
  if (@isset($_POST['import_pool']))
  {
   $result = zfs_pool_importable(false);
   $scannedforpools = true;
   if ($result['rv'] == 0)
   {
    $importarr = array();
    if (@is_array($result['importable']))
     foreach ($result['importable'] as $importable)
      $importarr[] = '<input type="submit" name="import_hidden_'
       .$importable['id'].'" '
       .'value="Import pool '.htmlentities($importable['pool']).'" /> ';
    $import_buttons = implode(' ', $importarr);
    $import_rawoutput = $result['output_str'];
    $importablepools = (empty($importarr)) ? false : true;
    if (!$importablepools)
    {
     $_SESSION['welcomewizard']['noimportablepools'] = true;
     $noimportablepools = true;
    }
   }
   elseif ($result['rv'] == 1)
   {
    $import_rawoutput = 'No importable pools have been found.';
    $_SESSION['welcomewizard']['noimportablepools'] = true;
    $noimportablepools = true;
   }
   else
    $import_rawoutput = '<span class="warning">WARNING: got return value '
     .$result['rv'].chr(10).$result['output_str'];
  }

  // hide pool import output box if no output is given
  $class_import = (strlen($import_rawoutput) > 0) ? 'normal' : 'hidden';

  // create pool button and text
  $w3_createpoolbutton = ($noimportablepools) ? '' : 'disabled="disabled"';
  $w3_createpooltext = ($noimportablepools) ? 'hidden' : 'normal';

  // advice box
  $pools = (empty($zpools)) ? false : true;
  $w3_advice_scan = (!$noimportablepools AND !$scannedforpools) ? 
   'normal' : 'hidden';
  $w3_advice_import = (!$noimportablepools AND $scannedforpools) ? 
   'normal' : 'hidden';
  $w3_advice_nopool = ($noimportablepools AND !$pools) ? 'normal' : 'hidden';
  $w3_advice_noboot = ($noimportablepools AND $pools AND !$bootablepools) ? 
   'normal' : 'hidden';
  $w3_advice_continue = ($noimportablepools AND $pools AND $bootablepools) ? 
   'normal' : 'hidden';
  $w3_advicebox = ($w3_advice_continue == 'normal') ? 'continue' : 'stop';
 }

 if ($class_w4 == 'normal')
 {
  $class_datasent = (@isset($_GET['datasent'])) ? 'normal' : 'hidden';
  $dmesg = file_get_contents('/var/run/dmesg.boot');
  $dpos = strrpos($dmesg, 'Copyright (c) 1992-2011 The FreeBSD Project.');
  $w4_dmesg = substr($dmesg, (int)$dpos);
 }

 if ($class_w5 == 'normal')
 {
  // fetch activation options
  $act = @$_SESSION['welcomewizard']['activation'];
  $act_feedback = @$_SESSION['welcomewizard']['feedback'];
  $act_feedback_text = @$_SESSION['welcomewizard']['feedback_text'];
  $uuid = @$_SESSION['welcomewizard']['uuid'];

  // classes
  $class_activation_success = ($act < 3 AND (strlen($uuid) > 0)) ? 'normal' 
   : 'hidden';
  $class_activation_skipped = ($act == 3) ? 'normal' : 'hidden';
  $class_activation_failure = ($act < 3 AND !$uuid) ? 'normal' : 'hidden';

  // finish welcome wizard (cause next pageview to be handled normally)
  if ((int)$act > 0)
   finish_welcomewizard();
 }

 // export new tags
 return @array(
  'TABLE_PHYSDISKS'	=> $physdisks,
  'TABLE_POOLLIST'	=> $poollist,
  'CLASS_WELCOME0'	=> $class_w0,
  'CLASS_WELCOME1'	=> $class_w1,
  'CLASS_WELCOME2'	=> $class_w2,
  'CLASS_WELCOME3'	=> $class_w3,
  'CLASS_WELCOME4'	=> $class_w4,
  'CLASS_WELCOME5'	=> $class_w5,
  'CLASS_IMPORTOUTPUT'	=> $class_import,
  'CLASS_DATASENT'	=> $class_datasent,
  'CLASS_ACTI_SUCCESS'	=> $class_activation_success,
  'CLASS_ACTI_SKIPPED'	=> $class_activation_skipped,
  'CLASS_ACTI_FAILURE'	=> $class_activation_failure,
  'W1_AC_1'		=> $w1_ac_1,
  'W1_AC_2'		=> $w1_ac_2,
  'W1_AC_3'		=> $w1_ac_3,
  'W1_NOAUTH'		=> $w1_noauth,
  'W1_AUTH'		=> $w1_auth,
  'W1_AUTH_PW'		=> $w1_auth_pw,
  'W1_IPADDR'		=> $w1_user_ipaddr,
  'W3_IMPORTABLE'	=> $import_buttons,
  'W3_IMPORTOUTPUT'	=> $import_rawoutput,
  'W3_SPALIST'		=> $poolspa,
  'W3_ZPLLIST'		=> $poolzpl,
  'W3_WHOLEDISKS'	=> $wholedisks,
  'W3_CREATEPOOLBUTTON'	=> $w3_createpoolbutton,
  'W3_CREATEPOOLTEXT'	=> $w3_createpooltext,
  'W3_ADVICEBOX'	=> $w3_advicebox,
  'W3_ADVICE_SCAN'	=> $w3_advice_scan,
  'W3_ADVICE_IMPORT'	=> $w3_advice_import,
  'W3_ADVICE_NOPOOL'	=> $w3_advice_nopool,
  'W3_ADVICE_NOBOOT'	=> $w3_advice_noboot,
  'W3_ADVICE_CONTINUE'	=> $w3_advice_continue,
  'W4_DMESG'		=> $w4_dmesg,
  'W5_ACTIVATION'	=> $act,
  'W5_FEEDBACK'		=> $act_feedback,
  'W5_FEEDBACK_TEXT'	=> $act_feedback_text
 );
}

function submit_welcome_submit_0()
{
 if (@isset($_POST['submit0']))
  redirect_url('?welcome1');
}

function submit_welcome_submit_1()
{
 global $guru;

 if (@isset($_POST['goback0']))
  redirect_url('?welcome0');
 elseif (@isset($_POST['skip_wizard']))
 {
  skip_welcomewizard();
  redirect_url('system.php?pref');
 }
 elseif (@isset($_POST['submit1']))
 {
  // sanity check: if authentication chosen password must be set
  if ($_POST['authentication'] == 2)
  {
   if (strlen($_POST['auth_pass1']) < 1)
    friendlyerror('if you select authentication, you must set a password', 
     '?welcome1');
   if ($_POST['auth_pass1'] != $_POST['auth_pass2'])
    friendlyerror('the password you chosen does not match the verification '
     .'password - please try again', '?welcome1');
  }
  // save settings
  $_SESSION['welcomewizard']['access_control'] = (int)$_POST['access_control'];
  $_SESSION['welcomewizard']['access_whitelist'][$_SERVER['REMOTE_ADDR']] = 
   $_SERVER['REMOTE_ADDR'];
  if ($_POST['authentication'] == 2)
   $_SESSION['welcomewizard']['authentication'] = (string)$_POST['auth_pass1'];
  else
   $_SESSION['welcomewizard']['authentication'] = '';
  // redirect
  redirect_url('?welcome2');
 }
}

function submit_welcome_submit_2()
{
 if (@isset($_POST['goback1']))
  redirect_url('?welcome1');
 elseif (@isset($_POST['submit2']))
  redirect_url('?welcome3');
}

function submit_welcome_submit_3()
{
 global $guru;

 // redirect URL
 $url = '?welcome3';

 if (@isset($_POST['goback2']))
  redirect_url('?welcome2');
 elseif (@isset($_POST['submit3']))
  redirect_url('?welcome4');
 elseif (@isset($_POST['submit_createnewzpool']))
 {
  activate_library('disk');
  activate_library('guru');
  activate_library('zfs');

  // create new ZFS pool
  $s = sanitize(@$_POST['new_zpool_name'], null, $poolname, 32);
  if (!$s)
   friendlyerror('please enter a valid pool name using only alphanumerical '
    .'+ underscore (_) + dash characters (-)', $url);
  $zpl = $_POST['new_zpool_zpl'];
  $spa = $_POST['new_zpool_spa'];
  $sectorsize = $_POST['new_zpool_sectorsize'];

  // scan for selected whole disks
  $wholedisks = array();
  foreach ($_POST as $var => $value)
   if (substr($var, 0, strlen('addwholedisk_')) == 'addwholedisk_')
    if ($value == 'on')
     $wholedisks[] = substr($var, strlen('addwholedisk_'));

  // sanity checks
  if (empty($wholedisks))
   friendlyerror('please select one or more disks to create a new pool', $url);

  // validate ZFS pool/filesystem version
  $sys = guru_zfsversion();
  $options_str = '';
  if (($spa > 0) AND ($spa <= $sys['spa']))
   $options_str .= '-o version='.$spa.' ';
  if (($zpl > 0) AND ($zpl <= $sys['zpl']))
   $options_str .= '-O version='.$zpl.' ';
  $options_str .= '-O atime=off ';

  // format disks with GPT
  $member_disks = array();
  foreach ($wholedisks as $disk)
  {
   // gather diskinfo
   $diskinfo = disk_info($disk);
   // gather GPT label from POST vars and validate
   $label = $poolname.'-disk'.(count($member_disks)+1);
   // reservespace is the space we leave unused at the end of GPT partition
   $reservespace = 1;
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

   // format disk
   $result = super_script('format_disk', $disk);
   if ($result['rv'] != 0)
    friendlyerror('Formatting disk '.$disk.' failed - perhaps it is in use?', 
     $url);

   // destroy existing GEOM label
   super_script('geom_label_destroy', $disk);

   // sanity check on label - this check should happen AFTER formatting!
   usleep(50000);
   if (file_exists('/dev/gpt/'.$label))
    friendlyerror('another disk exists with the name '.label
     .' - please choose another pool name!', $url);

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

   // create GPT partition scheme and redirect
   $result = super_script('create_gpt_partitions', $disk.' "'.$label.'" '
    .(int)$data_size.' '.$pmbr.' '.$gptzfsboot);
   if ($result['rv'] == 0)
    $member_disks[] = 'gpt/'.$label;
   else
    friendlyerror('could not format disk <b>'.$disk.'</b> - '
     .'perhaps it is already in use?', $url);
  }
  $member_count = (int)count($member_disks);

  // assemble member string (with .nop suffix if applicable)
  $member_str = '';
  foreach ($member_disks as $disklabel)
   if (is_numeric($sectorsize))
    $member_str .= $disklabel.'.nop ';
   else
    $member_str .= $disklabel.' ';

  // extract redundancy
  $redundancy = zfs_extractsubmittedredundancy($_POST['new_zpool_redundancy'],
   $member_count, $url);

  // mountpoint (same as / + poolname)
  $mountpoint = '/'.$poolname;

  // validate vdevs

  // create command array
  $commands = array();

  // handle sector size overrides
  // we do this by creating GNOP providers which override the sector size
  // this will force ashift to be different (inspect using zdb)
  // this also works across reboots, and the .nop providers are only needed once
  if (is_numeric($sectorsize))
   if (is_array($member_disks))
    foreach ($member_disks as $vdevdisk)
     $commands[] = '/sbin/gnop create -S '.(int)$sectorsize.' /dev/'.$vdevdisk;

  // create zpool create command
  $commands[] = '/sbin/zpool create '.$options_str.$poolname.' '.$redundancy.' '
   .$member_str;
  $commands[] = '/usr/sbin/chown -R nfs:nfs '.$mountpoint;

  // execute (GNOP + zpool create)
  foreach ($commands as $command)
  {
   $result = super_execute($command);
   if ($result['rv'] != 0)
    friendlyerror('failure trying to create pool (command = '
     .htmlentities($command).')', $url);
  }

  // finish
  page_feedback('a new pool has been created with the name <b>'.$poolname
    .'</b>!', 'b_success');
  redirect_url('?welcome3');
 }

 // scan POST variables for hidden or destroyed pool import buttons
 activate_library('zfs');
 $result = false;
 foreach ($_POST as $var => $value)
  if (substr($var, 0, strlen('import_hidden_')) == 'import_hidden_')
   $result = zfs_pool_import(substr($var, strlen('import_hidden_')), $url, 
    false);
 if ($result)
  friendlynotice('Pool imported - it should be visible now!', $url);
}

function submit_welcome_submit_4()
{
 if (@isset($_POST['goback3']))
  redirect_url('?welcome3');
 elseif (@isset($_POST['submit4']))
 {
  // required library
  activate_library('activation');
  // save settings
  if (((int)@$_POST['activation'] != 3) AND ((int)@$_POST['activation'] > 0))
  {
   $_SESSION['welcomewizard']['activation'] = (int)$_POST['activation'];
   $_SESSION['welcomewizard']['feedback'] = (int)$_POST['feedback'];
   $_SESSION['welcomewizard']['feedback_text'] = $_POST['feedback_text'];
   // activate now
   $_SESSION['welcomewizard']['uuid'] = activation_submit($_POST['activation'],
    (int)$_POST['feedback'], $_POST['feedback_text']);
  }
  else
   $_SESSION['welcomewizard']['activation'] = 3;
  // redirect to next step
  redirect_url('?welcome5');
 }
 elseif (@isset($_POST['skip_activation']))
 {
  // skip activation
  $_SESSION['welcomewizard']['activation'] = 3;
  $_SESSION['welcomewizard']['feedback'] = 
   @$_SESSION['welcomewizard']['feedback'];
  $_SESSION['welcomewizard']['feedback_text'] = 
   @$_SESSION['welcomewizard']['feedback_text'];
  // redirect to step 5 (finish)
  redirect_url('?welcome5');
 }
 else
  error('wrong submit option on step4');
}

function submit_welcome_submit_5()
{
 if (@isset($_POST['goback4']))
  redirect_url('?welcome4');
 elseif (@isset($_POST['submit5']))
  redirect_url('status.php');
}

function finish_welcomewizard()
// finishes welcome wizard and writes preferences according to chosen options
{
 global $guru;

 // fetch current preferences
 if (@is_array($guru['preferences']))
  $pref = $guru['preferences'];
 else
  $pref = $guru['default_preferences'];

 // restore old preferences (when user clicked Run welcome wizard again button)
 if (@isset($_SESSION['welcomewizard']['oldpreferences']))
  foreach ($pref as $var => $value)
   if (@isset($_SESSION['welcomewizard']['oldpreferences'][$var]))
    $pref[$var] = $_SESSION['welcomewizard']['oldpreferences'][$var];

 // access control
 $pref['access_control'] = (int)@$_SESSION['welcomewizard']['access_control'];
 $pref['access_whitelist'] = @$_SESSION['welcomewizard']['access_whitelist'];

 // authentication
 $pref['authentication'] = @$_SESSION['welcomewizard']['authentication'];

 // activation status
 $pref['uuid'] = @$_SESSION['welcomewizard']['uuid'];

 // write preferences
 procedure_writepreferences($pref);

 // activate preferences for this pageview
 $guru['preferences'] = $pref;

 // reset SESSION welcomewizard data
 unset($_SESSION['welcomewizard']);
}

function skip_welcomewizard()
// skips the welcome wizard
{
 global $guru;

 // fetch current preferences
 if (@is_array($guru['preferences']))
  $pref = $guru['preferences'];
 else
  $pref = $guru['default_preferences'];

 // restore old preferences (when user clicked Run welcome wizard again button)
 if (@!isset($_SESSION['welcomewizard']['oldpreferences']))
  page_feedback('created new configuration file containing default '
   .'preferences', 'b_success');
 else
  foreach ($pref as $var => $value)
   if (@isset($_SESSION['welcomewizard']['oldpreferences'][$var]))
    $pref[$var] = $_SESSION['welcomewizard']['oldpreferences'][$var];

 // write preferences
 procedure_writepreferences($pref);

 // reset SESSION welcomewizard data
 unset($_SESSION['welcomewizard']);
}

?>
