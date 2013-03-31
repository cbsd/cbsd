<?php

function content_system_install_step2()
{
 // required libraries
 activate_library('guru');
 activate_library('service');

 // mount media and retrieve system versions (DISABLED)
 //guru_mountmedia();

 // call functions
 $sysvers = guru_fetch_systemversions();
 $currentver = guru_fetch_current_systemversion();
 $cpuarch = guru_getsystemplatform();
 if (!is_array($sysvers))
  error('Could not fetch remote system versions; check internet connection!');

 // variables
 $dist = @$_GET['dist'];
 $displayobsolete = (@$_GET['displayobsolete']) ? true : false;
 $sysver_avail = false;
 $media_mounted = false;

 // scan for mounted media
 $cdrom_mp = scandir('/cdrom/');
 if (@count($cdrom_mp) > 2)
  $media_mounted = true;

 // download in progress
 $dip_direct = (guru_check_directdownload()) ? 'normal' : 'hidden';
 $dip_torrent = (guru_check_torrentdownload()) ? 'normal' : 'hidden';

 // page refresh
 $refresh = 5;
 if (($dip_direct == 'normal') OR ($dip_torrent == 'normal'))
  page_refreshinterval($refresh);

 // system version table
 $systemversions = array();
 // sort sysvers array by key
 krsort($sysvers);
 // output table row for every system version
 foreach ($sysvers as $data)
 {
  // system version name
  $name = htmlentities($data['name']);

  // exported variables
  $activerow = ($data['md5hash'] == $currentver['md5']) 
   ? 'class="activerow"' : '';
  $namelink = $name;
  $platform = guru_sysctl('hw.machine_arch');
  $torrentname = 'ZFSguru-system-'.$name.'-'.$platform;
  $size_human = @sizehuman($data['filesize'], 1);
  $size_binary = @sizebinary($data['filesize'], 1);
  $size_suffix = '';
  $size_class = '';
  $availability = 'Unavailable';
  $notes = (@strlen($data['notes']) > 0) ? '<a href="'.$data['notes'].'" '
   .'onclick="window.open(this.href,\'_blank\');return false;">notes</a>' : '-';

  // check availability of system image somewhere on disk or USB/CD media
  $available = guru_locate_systemimage($data['md5hash']);

  // hide system version if obsolete
  // unless currently running that version or mounted a LiveCD with that version
  if ((@$data['branch'] == 'obsolete') AND (!@isset($_GET['displayobsolete'])))
//   if ($data['md5hash'] != $currentver['md5'])
   if (!$available)
    continue;

  if ($available)
  {
   $sysver_avail = true;
   $availability = '<span class="green bold italic">Available</span>';
   $realsize = filesize($available);
   if (($dip_direct == 'normal') AND 
       (filesize($available) != $data['filesize']))
    $availability = '<span class="blue bold">Downloading</span>';
   elseif ($data['name'] == 'Unknown')
    $namelink = '<a href="system.php?install&dist='.$dist.'&sysver=HASH'
     .$data['md5hash'].'">'.htmlentities($data['name']).'</a>';
   else
    $namelink = '<a href="system.php?install&dist='.$dist.'&sysver='
     .$data['name'].'">'.htmlentities($data['name']).'</a>';
   // visual effects after and during downloading
   if ($realsize == $data['filesize'])
    $size_suffix = '(v)';
   else
    $size_class = 'class="activecell"';
  }
  elseif (guru_check_torrentdownload($torrentname))
  {
   $availability = '<span class="blue bold">Downloading Torrent</span>';
  }
  elseif ($data['name'] != 'Unknown')
   $availability = '<input type="submit" name="download_'.$data['name'].'" '
    .'value="Download '.$data['name'].'" />';

  // add row to table array
  $systemversions[] = array(
   'SYSVER_ACTIVEROW'	=> $activerow,
   'SYSVER_NAME'	=> $namelink,
   'SYSVER_BRANCH'	=> @$data['branch'],
   'SYSVER_BSDVERSION'	=> @$data['bsdversion'],
   'SYSVER_SPA'		=> @$data['spa'],
   'SYSVER_SIZE_CLASS'	=> $size_class,
   'SYSVER_SIZE_HUMAN'	=> $size_human,
   'SYSVER_SIZE_BINARY'	=> $size_binary,
   'SYSVER_SIZE_SUFFIX'	=> $size_suffix,
   'SYSVER_MD5'		=> @$data['md5hash'],
   'SYSVER_AVAIL'	=> $availability,
   'SYSVER_NOTES_URL'	=> $notes
  );
 }

 // display/hide obsolete system versions classes
 $class_obsolete_display = (@isset($_GET['displayobsolete'])) 
  ? 'hidden' : 'normal';
 $class_obsolete_hide = (@isset($_GET['displayobsolete']))
  ? 'normal' : 'hidden';

 // hintbox classes
 $class_avail = ($sysver_avail) ? 'normal' : 'hidden';
 $class_notavail = ($sysver_avail) ? 'hidden' : 'normal';
 $class_mountcd = ($media_mounted) ? 'hidden' : 'normal';
 $class_unmountcd = ($media_mounted) ? 'normal' : 'hidden';

 // export new tags
 return array(
  'PAGE_ACTIVETAB'			=> 'Install',
  'PAGE_TITLE'				=> 'Install (step 2)',
  'TABLE_INSTALL_SYSTEMVERSIONS'	=> $systemversions,
  'CLASS_DIP_DIRECT'                    => $dip_direct,
  'CLASS_DIP_TORRENT'                   => $dip_torrent,
  'CLASS_OBSOLETE_DISPLAY'		=> $class_obsolete_display,
  'CLASS_OBSOLETE_HIDE'			=> $class_obsolete_hide,
  'CLASS_AVAIL'				=> $class_avail,
  'CLASS_NOTAVAIL'			=> $class_notavail,
  'CLASS_MOUNTCD'			=> $class_mountcd,
  'CLASS_UNMOUNTCD'			=> $class_unmountcd,
  'INSTALL_DIST'			=> $dist,
  'INSTALL_CURRENT_DIST'		=> $currentver['dist'],
  'INSTALL_CURRENT_SYSVER'		=> $currentver['sysver'],
  'INSTALL_CURRENT_MD5'			=> $currentver['md5'],
  'INSTALL_CPUARCH'			=> $cpuarch
 );
}

function submit_system_install_download()
{
 global $guru;

 // required library
 activate_library('guru');

 // figure out which system version to download
 $sysver = false;
 foreach ($_POST as $value)
  if (substr($value, 0, strlen('Download ')) == 'Download ')
   $sysver = substr($value, strlen('Download '));
 if (@strlen($sysver) < 1)
  error('Invalid system version download request');
 // fetch system versions
 $currentver = guru_fetch_current_systemversion();
 $sysvers = guru_fetch_systemversions();
 // check if version exists
 if (!@isset($sysvers[$sysver]))
  error('Unknown system version "'.$sysver.'"');
 // check for free disk space (LiveCD is bound by RAM)
 $reserved = 20 * 1024 * 1024;
 if (disk_free_space($guru['tempdir']) < 
    (@$sysvers[$sysver]['filesize'] + $reserved))
  if ($currentver['dist'] == 'livecd')
   error('Insufficient free memory; '
    .'LiveCD is bound by RAM size; add more RAM!');
  else
   error('Insufficient free space available; '
    .'need more space on '.$guru['tempdir']);
 // download system version - grab all data first
 $url = @trim($sysvers[$sysver]['url']);
 $torrent = @trim($sysvers[$sysver]['torrent']);
 $sysname = @trim(substr($url, strrpos($url, '/')+1));
 $syssize = (int)@$sysvers[$sysver]['filesize'];
 $md5 = @trim($sysvers[$sysver]['md5hash']);
 $sha1 = @trim($sysvers[$sysver]['sha1hash']);
 // prefer torrent downloads to http
 $prefer_torrent = (@$guru['preferences']['download_method'] == 'torrent') ? 
  true : false;

 if (((strlen($torrent) > 5) AND
      ($prefer_torrent) AND
      (is_dir($guru['torrent']['path_torrents'])))
     OR
     ((strlen($torrent) > 5) AND
      (strlen($url) < 5) AND
      (is_dir($guru['torrent']['path_torrents']))))
 {
  // torrent download
  activate_library('torrent');
  $result = torrent_download($torrent);
 }
 elseif (strlen($url) > 5)
 {
  // direct HTTP download
  exec($guru['docroot'].'/scripts/download_systemversion.sh '
   .'"'.$url.'" > /dev/null', $result, $rv);
  if ($rv == 0)
  {
   // write checksum files
   file_put_contents($guru['tempdir'].'/'.$sysname.'.md5', $md5);
   file_put_contents($guru['tempdir'].'/'.$sysname.'.sha1', $sha1);
  }
 }
 else
  error('there are no valid download sources available!');
 // redirect on success but delay for 6 seconds for download detection
 sleep(6);
 redirect_url('system.php?install&dist='.@$_POST['dist']);
}

function submit_system_install_changemedia()
{
 // required library
 activate_library('guru');

 if (@isset($_POST['mountmedia']))
 {
  // mount media
  $result = guru_mountmedia();
  // redirect
  $url = 'system.php?install&dist='.@$_POST['dist'];
  if ($result)
   friendlynotice('your media device was successfully <b>mounted</b>!', $url);
  else
   friendlyerror('could not find a ZFSguru media device (LiveCD/Embedded)', 
    $url);
 }
 elseif (@isset($_POST['unmountmedia']))
 {
  // unmount media
  $result = guru_unmountmedia();
  // redirect
  $url = 'system.php?install&dist='.@$_POST['dist'];
  if ($result)
   friendlynotice('your media is successfully <b>unmounted</b>', $url);
  else
   friendlyerror('could not unmount your media device', $url);
 }
}

?>
