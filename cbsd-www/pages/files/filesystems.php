<?php

function content_files_filesystems()
{
 // import zfs lib
 activate_library('zfs');

 // set active tab
 $page['activetab'] = 'Filesystems';

 // call function
 $zfslist = zfs_filesystem_list();
 if (!is_array($zfslist))
  $zfslist = array();

 // keep track of ZFSguru specific filesystems
 $gurufs = false;
 $hidegurufs = (@isset($_GET['displaygurufs'])) ? false : true;
 $displaygurufs = ($hidegurufs) ? '' : '&displaygurufs';

 // construct filesystem list table
 $queryfs = 'XXX';
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
    .str_replace('%2F','/',urlencode($fsdata['mountpoint']))
    .'">'.htmlentities($fsdata['mountpoint']).'</a>';
  $i++;
 }

 // filesystem selectbox
 $fsselectbox = '';
 if (is_array($zfslist))
  foreach ($zfslist as $fsname => $fsdata)
   $fsselectbox .= '<option value="'.htmlentities($fsname).'">'
    .htmlentities($fsname).'</option>';

 // display/hide gurufs class
 $class_gurufs = ($gurufs) ? 'normal' : 'hidden';
 $class_gurufs_display = ($hidegurufs) ? 'normal' : 'hidden';
 $class_gurufs_hide = (!$hidegurufs) ? 'normal' : 'hidden';

 // export new tags
 return array(
  'PAGE_ACTIVETAB'		=> 'Filesystems',
  'PAGE_TITLE'			=> 'Filesystems',
  'TABLE_FILES_FSLIST'		=> $fslist,
  'CLASS_GURUFS'		=> $class_gurufs,
  'CLASS_GURUFS_DISPLAY'	=> $class_gurufs_display,
  'CLASS_GURUFS_HIDE'		=> $class_gurufs_hide,
  'DISPLAYGURUFS'		=> $displaygurufs,
  'FILES_FSSELECTBOX'		=> $fsselectbox
 );
}

function submit_filesystem_create()
{
 // POST vars
 $s = sanitize(@$_POST['create_fs_name'], null, $fsname, 32);
 $parent = @$_POST['create_fs_on'];
 $fspath = $parent.'/'.$fsname;
 $url = 'files.php';
 $url2 = 'files.php?query='.$fspath;

 // sanity check
 if (!$s)
  friendlyerror('please enter a valid name for the new filesystem, use only '
   .'alphanumerical + _ + - characters with a maximum length of 32', $url);
 if (strlen($parent) < 1)
  friendlyerror('please select a valid parent filesystem', $url);

 // execute
 $commands = array(
  '/sbin/zfs create '.$fspath,
  '/usr/sbin/chown -R nfs:nfs /'.$fspath
 );
 dangerouscommand($commands, $url2);
}

?>
