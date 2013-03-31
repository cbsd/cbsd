<?php

function content_files_query()
{
 // required libraries
 activate_library('guru');
 activate_library('samba');
 activate_library('zfs');

 // set active tab
 $page['activetab'] = 'Filesystems';

 // call function
 $zfslist = zfs_filesystem_list();
 $zfsversion = guru_zfsversion();

 // keep track of ZFSguru specific filesystems
 $gurufs = false;
 $hidegurufs = (@isset($_GET['displaygurufs'])) ? false : true;
 $displaygurufs = ($hidegurufs) ? '' : '&displaygurufs';

 // queried filesystem
 $queryfs = @htmlentities(trim($_GET['query']));

 // redirect if user queried unknown filesystem
 if (@!isset($zfslist[$queryfs]))
  friendlyerror('unknown filesystem "'.$queryfs.'"', 'files.php');

 // construct filesystem list table
 $fslist = array();
 $i = 0;
 foreach ($zfslist as $fsname => $fsdata)
 {
  // determine whether fs is system filesystem
  $fsbase = @substr($fsname, strpos($fsname, '/') + 1);
  if ($basepos = strpos($fsbase, '/'))
   $fsbase = @substr($fsbase, 0, $basepos);
  if (($fsbase == 'zfsguru') OR
      (substr($fsbase, 0, strlen('zfsguru-system')) == 'zfsguru-system') OR
      ($fsbase == 'SWAP001'))
  {
   // zfsguru specific filesystem
   $gurufs = true;
   if ($hidegurufs)
    continue;
   else
    $fslist[$i]['FS_CLASS'] = 'filesystem_system ';
  }
  $fslist[$i]['FS_ESC'] = $fsname;
  $fslist[$i]['FS_USED'] = $fsdata['used'];
  $fslist[$i]['FS_AVAIL'] = $fsdata['avail'];
  $fslist[$i]['FS_REFER'] = $fsdata['refer'];
  $fslist[$i]['FS_CLASS'] = @$fslist[$i]['FS_CLASS'];
  if (strpos($fsname, '/') === false)
   $fslist[$i]['FS_CLASS'] = 'filesystem_root ';
  if ($fsname == @$queryfs)
   $fslist[$i]['FS_CLASS'] .= 'filesystem_selected';
  else
   $fslist[$i]['FS_CLASS'] .= 'normal';
  if ($fsdata['mountpoint'] == 'legacy')
   $fslist[$i]['FS_MOUNTPOINT'] = '<i>legacy</i>';
  elseif ($fsdata['mountpoint'] == '-')
   $fslist[$i]['FS_MOUNTPOINT'] = '<i>volume</i>';
  else
   $fslist[$i]['FS_MOUNTPOINT'] = '<a href="files.php?browse='
    .str_replace('%2F', '/', urlencode($fsdata['mountpoint']))
    .'">'.htmlentities($fsdata['mountpoint']).'</a>';
  $i++;
 }

 // filesystem selectbox
 $fsselectbox = '';
 foreach ($zfslist as $fsname => $fsdata)
  if ($fsname == $queryfs)
   $fsselectbox .= '<option value="'.htmlentities($fsname)
    .'" selected="selected">'.htmlentities($fsname).'</option>';
  else
   $fsselectbox .= '<option value="'.htmlentities($fsname).'">'
    .htmlentities($fsname).'</option>';

 // display/hide gurufs class
 $class_gurufs = ($gurufs) ? 'normal' : 'hidden';
 $class_gurufs_display = ($hidegurufs) ? 'normal' : 'hidden';
 $class_gurufs_hide = (!$hidegurufs) ? 'normal' : 'hidden';

 // filesystem query data
 if (@strlen($queryfs) > 0)
 {
  // figure out which pool this filesystem belongs to
  if (strpos($queryfs, '/') === false)
   $poolname = $queryfs;
  else
   $poolname = substr($queryfs, 0, 
    strpos($queryfs, '/'));

  // gather pool SPA version and system version
  $pool_spa = zfs_pool_version($poolname);

  // gather all filesystem properties of queried filesystem
  $zfsinfo = zfs_filesystem_getproperties($queryfs);
 }

 // process data for queried filesystem
 if (@count($zfsinfo) > 1)
 {
  // calculate all data for queried filesystem
  $created = $zfsinfo['info']['creation'][2].' '.
   $zfsinfo['info']['creation'][3].' '.
   $zfsinfo['info']['creation'][4].' '.$zfsinfo['info']['creation'][6].' @ '.
   $zfsinfo['info']['creation'][5];
  $compressedsize = $zfsinfo['info']['referenced'][2];
  $compressfactor = (double)$zfsinfo['info']['compressratio'][2];
  $compressratio = number_format($compressfactor, 2, '.', '');
  $uncompressedsize = round((double)$zfsinfo['info']['referenced'][2] *
   $compressfactor, 2);
  $uncompressedsize .= $zfsinfo['info']['referenced'][2]{
   strlen($zfsinfo['info']['referenced'][2])-1};
  $spacesaved = number_format((double)$zfsinfo['info']['referenced'][2] * 
   ($compressfactor-1), 2, '.', '');
  $snapused = $zfsinfo['info']['usedbysnapshots'][2];
  // todo: this could use some work
  $spacesaved .= $zfsinfo['info']['used'][2]{strlen(
   $zfsinfo['info']['used'][2])-1};
  $childrenused = $zfsinfo['info']['usedbychildren'][2];
  $sizeavailable = $zfsinfo['info']['available'][2];
  $totalsize = $zfsinfo['info']['used'][2];
  // checkboxes
  $atime = @trim($zfsinfo['settings']['atime'][2]);
  $readonly = @trim($zfsinfo['settings']['readonly'][2]);
  $cb_atime = ($atime == 'on')
   ? 'checked="checked"' : '';
  $cb_readonly = ($readonly == 'on')
   ? 'checked="checked"' : '';
  // select boxes
  $compressiontypes = array(
   'off'	=> 'No compression',
   'lzjb'	=> 'LZJB (light compression; fastest)',
   'gzip-1'	=> 'GZIP-1',
   'gzip-2'	=> 'GZIP-2',
   'gzip-3'	=> 'GZIP-3',
   'gzip-4'	=> 'GZIP-4',
   'gzip-5'	=> 'GZIP-5',
   'gzip'	=> 'GZIP-6 (default gzip)',
   'gzip-7'	=> 'GZIP-7',
   'gzip-8'	=> 'GZIP-8',
   'gzip-9'	=> 'GZIP-9 (best compression; slowest)');
  $deduptypes = array(
   'off'		=> 'No deduplication',
   'on'			=> 'Fletcher4',
   'verify'		=> 'Fletcher4 +verify',
   'sha256'		=> 'SHA256',
   'sha256,verify'	=> 'SHA256 +verify');
  $copiestypes = array(
   '1' => 'No additional redundancy',
   '2' => 'Two copies of each file',
   '3' => 'Three copies of each file'
  );
  $checksumtypes = array(
   'off'	=> 'Disabled (NOT recommended!)',
   'on'		=> 'Fletcher2 (default)',
   'fletcher4'	=> 'Fletcher4',
   'sha256'	=> 'SHA256 (high CPU)'
  );

  $compression = @trim($zfsinfo['settings']['compression'][2]);
  $dedup = @trim($zfsinfo['settings']['dedup'][2]);
  $copies = @trim($zfsinfo['settings']['copies'][2]);
  $checksum = @trim($zfsinfo['settings']['checksum'][2]);
  $checksum = ($checksum == 'fletcher2') ? 'on' : $checksum;

  $box_compression = '';
  foreach ($compressiontypes as $value => $description)
   if ($value == $compression)
    $box_compression .= '<option value="'.$value.'" selected="selected">'
     .htmlentities($description).'</option>'.chr(10);
   else
    $box_compression .= '<option value="'.$value.'">'
     .htmlentities($description).'</option>'.chr(10);
  $box_dedup = '';
  foreach ($deduptypes as $value => $description)
   if ($value == $dedup)
    $box_dedup .= '<option value="'.$value.'" selected="selected">'
     .htmlentities($description).'</option>'.chr(10);
   else
    $box_dedup .= '<option value="'.$value.'">'
     .htmlentities($description).'</option>'.chr(10);
  $box_copies = '';
  foreach ($copiestypes as $value => $description)
   if ($value == $copies)
    $box_copies .= '<option value="'.$value.'" selected="selected">'
     .htmlentities($description).'</option>'.chr(10);
   else
    $box_copies .= '<option value="'.$value.'">'
     .htmlentities($description).'</option>'.chr(10);
  $box_checksum = '';
  foreach ($checksumtypes as $value => $description)
   if ($value == $checksum)
    $box_checksum .= '<option value="'.$value.'" selected="selected">'
     .htmlentities($description).'</option>'.chr(10);
   else
    $box_checksum .= '<option value="'.$value.'">'
     .htmlentities($description).'</option>'.chr(10);

  // disable deduplication if not supported by the system or pool
  $class_dedup = 'hidden';
  $class_nodedup_system = 'hidden';
  $class_nodedup_pool = 'hidden';
  if ($zfsversion['spa'] < 21)
   $class_nodedup_system = 'normal';
  elseif ($pool_spa < 21)
   $class_nodedup_pool = 'normal';
  else
   $class_dedup = 'normal';

  // samba share status
  if (@$zfsinfo['settings']['mountpoint'][2] == 'legacy')
  {
   // legacy mountpoint; skip sharing
   $mountpoint			= 'legacy';
   $mountpoint_string		= '<i>legacy</i>';
   $class_nfsshared		= 'hidden';
   $class_nfsnotshared		= 'normal';
   $nfssharestatus		= 'Not shared';
   $nfssharename		= '--legacy--';
   $nfsshareaction		= 'legacy';
   $nfsshareactionname		= 'legacy';
   $nfssharesubmit		= 'disabled="disabled"';
   $class_sambashared		= 'hidden';
   $class_sambanotshared	= 'normal';
   $sambasharestatus		= 'Not shared';
   $sambasharename		= '--legacy--';
   $sambashareaction		= 'legacy';
   $sambashareactionname	= 'legacy';
   $sambasharesubmit		= 'disabled="disabled"';
  }
  elseif (@$zfsinfo['info']['type'][2] == 'volume')
  {
   // ZVOL; skip sharing
   $mountpoint			= 'volume';
   $mountpoint_string		= '<i>volume</i>';
   $class_nfsshared		= 'hidden';
   $class_nfsnotshared		= 'normal';
   $nfssharestatus		= 'Not shared';
   $nfssharename		= '--volume--';
   $nfsshareaction		= 'volume';
   $nfsshareactionname		= 'volume';
   $nfssharesubmit		= 'disabled="disabled"';
   $class_sambashared		= 'hidden';
   $class_sambanotshared	= 'normal';
   $sambasharestatus		= 'Not shared';
   $sambasharename		= '--volume--';
   $sambashareaction		= 'volume';
   $sambashareactionname	= 'volume';
   $sambasharesubmit		= 'disabled="disabled"';
  }
  else
  {
   // normal filesystem

   // mountpoint
   $mountpoint = @trim($zfsinfo['settings']['mountpoint'][2]);
   $mountpoint_string = '<input class="yellow" type="text" name="mountpoint" '
    .'value="'.htmlentities($mountpoint).'" /> '
    .'<span class="minortext"><a href="/files.php?browse='
    .urlencode($mountpoint).'">[browse]</a></span>';

   // check if mountpoint is shared with Samba
   $sambasharename = samba_isshared($mountpoint);
   if ($sambasharename)
   {
    $class_sambashared		= 'normal';
    $class_sambanotshared	= 'hidden';
    $sambasharestatus		= 'Shared';
    $sambasharename		= $sambasharename;
    $sambashareaction		= 'unshare';
    $sambashareactionname	= 'Unshare';
   }
   else
   {
    $class_sambashared		= 'hidden';
    $class_sambanotshared	= 'normal';
    $sambasharestatus		= 'Not shared';
    $sambasharename		= '';
    $sambashareaction		= 'share';
    $sambashareactionname	= 'Share';
   }
   // nfs share status
   $nfssharename = @htmlentities($zfsinfo['settings']['sharenfs'][2]);
   if ($nfssharename == 'off')
   {
    $class_nfsshared	= 'hidden';
    $class_nfsnotshared	= 'normal';
    $nfssharestatus	= 'Not shared';
    $nfssharename	= $nfssharename;
    $nfsshareaction	= 'share';
    $nfsshareactionname	= 'Share';
   }
   else
   {
    $class_nfsshared	= 'normal';
    $class_nfsnotshared	= 'hidden';
    $nfssharestatus	= 'Shared';
    $nfssharename	= $nfssharename;
    $nfsshareaction	= 'unshare';
    $nfsshareactionname	= 'Unshare';
   }
  }
 }
 // other
 $defaultsnapshotname = date('Ymd');

 // export new tags
 $newtags = @array(
  'PAGE_ACTIVETAB'			=> 'Filesystems',
  'PAGE_TITLE'				=> 'Filesystem '.$queryfs,
  'FILES_FSSELECTBOX'			=> $fsselectbox,
  'TABLE_FILES_FSLIST'			=> $fslist,
  'CLASS_GURUFS'			=> $class_gurufs,
  'CLASS_GURUFS_DISPLAY'		=> $class_gurufs_display,
  'CLASS_GURUFS_HIDE'			=> $class_gurufs_hide,
  'DISPLAYGURUFS'			=> $displaygurufs,
  'QUERYFS'				=> $queryfs,
  'QUERYFS_CREATED'			=> $created,
  'QUERYFS_COMPRESSRATIO'		=> $compressratio,
  'QUERYFS_SIZE_UNCOMPRESSED'		=> $uncompressedsize,
  'QUERYFS_SIZE_COMPRESSED'		=> $compressedsize,
  'QUERYFS_SIZE_RECLAIMED'		=> $spacesaved,
  'QUERYFS_SIZE_SNAPSHOTS'		=> $snapused,
  'QUERYFS_SIZE_CHILDREN'		=> $childrenused,
  'QUERYFS_SIZE_TOTAL'			=> $totalsize,
  'QUERYFS_SIZE_AVAILABLE'		=> $sizeavailable,
  'QUERYFS_MOUNTPOINT'			=> $mountpoint,
  'QUERYFS_MOUNTPOINT_STRING'		=> $mountpoint_string,
  'QUERYFS_COMPRESSION'			=> $compression,
  'QUERYFS_DEDUP'			=> $dedup,
  'QUERYFS_COPIES'			=> $copies,
  'QUERYFS_CHECKSUM'			=> $checksum,
  'QUERYFS_ATIME'			=> $atime,
  'QUERYFS_READONLY'			=> $readonly,
  'QUERYFS_CHECKED_ATIME'		=> $cb_atime,
  'QUERYFS_CHECKED_READONLY'		=> $cb_readonly,
  'QUERYFS_COMPRESSIONOPTIONS'		=> $box_compression,
  'CLASS_DEDUP'				=> $class_dedup,
  'QUERYFS_DEDUP_OPTIONS'		=> $box_dedup,
  'CLASS_NODEDUP_SYSTEM'		=> $class_nodedup_system,
  'CLASS_NODEDUP_POOL'			=> $class_nodedup_pool,
  'QUERYFS_REDUNDANCYOPTIONS'		=> $box_copies,
  'QUERYFS_CHECKSUMOPTIONS'		=> $box_checksum,
  'QUERYFS_DEFAULTSNAPSHOTNAME'		=> $defaultsnapshotname,
  'CLASS_NFSSHARED'			=> $class_nfsshared,
  'CLASS_NFSNOTSHARED'			=> $class_nfsnotshared,
  'QUERYFS_NFSSHARESTATUS'		=> $nfssharestatus,
  'QUERYFS_NFSSHARENAME'		=> $nfssharename,
  'QUERYFS_NFSSHAREACTION'		=> $nfsshareaction,
  'QUERYFS_NFSSHAREACTIONNAME'		=> $nfsshareactionname,
  'QUERYFS_NFSSHARESUBMIT'		=> @$nfssharesubmit,
  'CLASS_SAMBASHARED'			=> $class_sambashared,
  'CLASS_SAMBANOTSHARED'		=> $class_sambanotshared,
  'QUERYFS_SAMBASHARESTATUS'		=> $sambasharestatus,
  'QUERYFS_SAMBASHARENAME'		=> $sambasharename,
  'QUERYFS_SAMBASHAREACTION'		=> $sambashareaction,
  'QUERYFS_SAMBASHAREACTIONNAME'	=> $sambashareactionname,
  'QUERYFS_SAMBASHARESUBMIT'		=> @$sambasharesubmit,
 );

 // return as tags
 return $newtags;
}

function submit_filesystem_modify()
{
 // variables
 $fs = @$_POST['fs_name'];
 $url = 'files.php?query='.$fs;
 $url2 = 'files.php';

 // submit actions
 if (@isset($_POST['submit_destroyfilesystem']))
 {
  // required library
  activate_library('samba');
  activate_library('zfs');
  // check for children filesystems
  $fslist = zfs_filesystem_list($fs, '-r -t all');
  // redirect to different page in case of children datasets
  if (count($fslist) > 1)
   redirect_url('files.php?destroy='.urlencode($fs));
  // no children; continue deletion of single filesystem
  $fs_mp = @current($fslist);
  $fs_mp = @$fs_mp['mountpoint'];
  // remove any samba shares on the mountpoint
  samba_removesharepath($fs_mp);
  // start command array
  $command = array();
  // check for SWAP filesystem
  exec('/sbin/swapctl -l', $swapctl_raw);
  $swapctl = @implode(chr(10), $swapctl_raw);
  $fsdetails = zfs_filesystem_getproperties($fs, 'org.freebsd:swap');
  if (@$fsdetails['org.freebsd:swap'][2] == 'on')
   if (@strpos($swapctl, '/dev/zvol/'.$fs) !== false)
    $command[] = '/sbin/swapoff /dev/zvol/'.$fs;
  // display message if swap volumes detected
  if (count($command) > 0)
   page_feedback('this volume is in use as SWAP device! '
    .'If you continue, the SWAP device will be deactivated first.', 'c_notice');
  // add destroy command
  $command[] = '/sbin/zfs destroy '.$fs;
  // defer to dangerous command function
  dangerouscommand($command, $url2);
 }
 elseif (@isset($_POST['submit_createsnapshot']))
 {
  sanitize(@$_POST['snapshot_name'], null, $snapname, 32);
  if (strlen($snapname) > 0)
   dangerouscommand('/sbin/zfs snapshot '.$fs.'@'.$snapname, $url);
  else
   friendlyerror('invalid snapshot name', $url);
 }
 elseif (@isset($_POST['submit_nfs_share']))
 {
  $share = @(($_POST['nfs_name'] == 'off') OR (strlen($_POST['nfs_name']) < 1))
   ? 'on' : $_POST['nfs_name'];
  dangerouscommand('/sbin/zfs set sharenfs="'.$share.'" '.$fs, $url);
 }
 elseif (@isset($_POST['submit_nfs_unshare']))
  dangerouscommand('/sbin/zfs set sharenfs="off" '.$fs, $url);
 elseif (@isset($_POST['submit_samba_share']))
 {
  // use supplied share name or create from filesystem name
  if (strlen($_POST['samba_name']) > 12)
   friendlyerror('samba name may not be longer than 12 characters', $url);
  elseif (strlen($_POST['samba_name']) > 0)
  {
   if (!sanitize(@$_POST['samba_name'], 'a-zA-Z0-9', $name, 12))
    friendlyerror('illegal character in samba name; please use only '
     .'alphanumerical characters (a-zA-Z0-9) and no spaces', $url);
  }
  elseif ($fsstripped = strrchr($fs, '/'))
   sanitize(substr($fsstripped, 1), 'a-zA-Z0-9', $name, 12);
  else
   sanitize($fs, 'a-zA-Z0-9', $name, 12);
  // final sanity check
  if (strlen($name) < 1)
   friendlyerror('failed creating a samba name for this filesystem', $url);

  // mountpoint
  $mp = @$_POST['fs_mountpoint'];
  if (strlen($mp) < 1)
   friendlyerror('invalid mountpoint detected; cannot continue', $url);

  // fetch samba configuration
  activate_library('samba');
  $sambaconf = samba_readconfig();
  // append new share to configuration
  $newshare = array(
   'path'	=> $mp,
   'browseable'	=> 'yes',
   'writeable'	=> 'yes',
   'public'	=> 'yes'
  );
  if (!@isset($sambaconf['shares'][$name]))
   $sambaconf['shares'][$name] = $newshare;
  else
   friendlyerror('A share with the name <b>'.htmlentities($name)
    .'</b> already exists; please choose another name!', $url);
  // write samba configuration
  $result = samba_writeconfig($sambaconf);
  if ($result)
   page_feedback('filesystem <b>'.htmlentities($fs).'</b> shared under the '
    .'name <b>'.$name.'</b>', 'b_success');
  else
   page_feedback('could not save Samba configuration while adding share!',
    'a_failure');
 }
 elseif (@isset($_POST['submit_samba_unshare']))
 {
  // no sanity required
  $name = @$_POST['samba_name'];
  // fetch samba configuration
  activate_library('samba');
  $sambaconf = samba_readconfig();
  if (@isset($sambaconf['shares'][$name]))
  {
   unset($sambaconf['shares'][$name]);
   $result = samba_writeconfig($sambaconf);
   if ($result)
    page_feedback('filesystem <b>'.htmlentities($fs).'</b> removed from '
     .'Samba shares', 'b_success');
   else
    page_feedback('could not save Samba configuration while removing share!',
     'a_failure');
  }
 }
 elseif (@isset($_POST['submit_updateproperties']))
 {
  // skip mountpoint for zvol or legacy filesystem
  if (($_POST['fs_mountpoint'] == 'legacy') OR 
      ($_POST['fs_mountpoint'] == 'volume'))
   $stringvars = array('compression', 'dedup', 'copies', 'checksum');
  else
   $stringvars = array('mountpoint', 'compression', 'dedup', 'copies', 
    'checksum');
  $boolvars = array('atime', 'readonly');
  // check all above variables for submitted information and act accordingly
  foreach ($stringvars as $var)
   if ((@strlen($_POST['fs_'.$var]) > 0) AND
    (@$_POST[$var] != $_POST['fs_'.$var]))
    dangerouscommand('/sbin/zfs set '.$var.'='.$_POST[$var].' '.$fs, $url);
  foreach ($boolvars as $var)
   if ((@$_POST[$var] == 'on') AND ($_POST['fs_'.$var] == 'off'))
    dangerouscommand('/sbin/zfs set '.$var.'=on '.$fs, $url);
   elseif ((@$_POST[$var] != 'on') AND ($_POST['fs_'.$var] == 'on'))
    dangerouscommand('/sbin/zfs set '.$var.'=off '.$fs, $url);
  friendlynotice('no options were updated', $url);
 }

 redirect_url($url);
}

?>

