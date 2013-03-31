<?php

function content_services_install()
{
 global $guru;

 // required libraries
 activate_library('guru');
 activate_library('service');

 // read database
 $db = service_db();

 // debug
 if (@isset($_GET['debug']))
  viewarray($db);

 // hide all classes at first
 $class_cat		= 'hidden';
 $class_services	= 'hidden';
 $class_infopage	= 'hidden';

 // navigation
 $nav_catshort		= trim(@$_GET['cat']);
 $nav_catlong		= @$db['categories'][$nav_catshort]['longname'];
 $nav_svcshort		= trim(@$_GET['service']);
 $nav_svclong		= @$db['services'][$nav_svcshort]['longname'];
 $class_hascat		= ($nav_catshort) ? 'normal' : 'hidden';
 $class_hassvc		= ($nav_svcshort) ? 'normal' : 'hidden';

 // tables
 $table_categories = array();
 $table_services = array();
 $table_infopage = array();

 // check which table to display (1: categories, 2: services, 3: infopage)
 if (@isset($_GET['service']))
 {
  // infopage
  $class_infopage = 'normal';

  // set category navigation
  $svc = trim($_GET['service']);
  $nav_catshort = @$db['services'][$svc]['cat'];
  $nav_catlong = @$db['categories'][$nav_catshort]['longname'];
  $class_hascat = ($nav_catshort) ? 'normal' : 'hidden';

  // infopage image
  $infopage_img = @$db['services'][$svc]['image'];

  // call functions
  $slist = service_list();
  $remotever = guru_fetch_systemversions();
  $curver = guru_fetch_current_systemversion();
  $platform = guru_getsystemplatform();

  // current installation status
  $installed = (@isset($slist[$svc])) ? true : false;

  // system image availability
  $sysimg_str = @$curver['sysver'];
  $sysimg_all = @$db['services'][$svc]['sysimg'];
  $sysimg = @$sysimg_all[$sysimg_str][$platform];
  $avail = (strlen(@$sysimg['version']) > 0) ? true : false;
  $infopage_downsize = sizebinary(@$sysimg['size'], 1);

  // download in progress
  $dip_http = guru_check_directdownload();
  $dip_torrent = guru_check_torrentdownload(@$sysimg['filename']);
  $dip = ($dip_http == true OR $dip_torrent == true) ? true : false;

  // page refresh
  $page_refresh = 5;
  if ($dip)
   page_refreshinterval($page_refresh);

  // check file availability
  $path_http = guru_locate_download(@$sysimg['filename'], $guru['tempdir']);
  $path_torrent = guru_locate_download(@$sysimg['filename'], 
   $guru['torrent']['path_finished']);
  $fileavail = ($path_http != false OR $path_torrent != false) ? true : false;

  // upgrade status (check version)
  $canupgrade = false;
  if ($installed)
  {
   $current_ver = @$slist[$svc]['version'];
   $new_ver = @$sysimg['version'];
   $canupgrade = ($new_ver == $current_ver) ? false : true;
  }

  // classes
  $ip_class_img = (strlen($infopage_img) > 0) ? 'normal' : 'hidden';
  $ip_class_installed = ($installed AND $avail) ? 'normal' : 'hidden';
  $ip_class_installedunavail = ($installed AND !$avail) ? 'normal' : 'hidden';
  $ip_class_downloading = ($dip) ? 'normal' : 'hidden';
  $ip_class_notinstalled1 = (!$installed AND !$dip AND !$fileavail AND $avail) 
   ? 'normal' : 'hidden';
  $ip_class_notinstalled2 = (!$installed AND !$dip AND $fileavail AND $avail)
   ? 'normal' : 'hidden';
  $ip_class_notinstalledunavail = (!$installed AND !$avail) 
   ? 'normal' : 'hidden';

  // display table of supported system versions in case service is unavailable
  if ($ip_class_notinstalledunavail == 'normal')
   if (is_array($sysimg_all))
    foreach ($sysimg_all as $sysver => $platformdata)
     if (is_array($platformdata))
      foreach ($platformdata as $tplatform => $data)
       $table_infopage[] = array(
        'IP_SVCLONG'	=> htmlentities($nav_svclong),
        'IP_VERSION'	=> @$data['version'],
        'IP_SIZE'	=> sizebinary(@$data['size'], 1),
        'IP_SYSVER'	=> $sysver,
        'IP_BRANCH'	=> @$remotever[$sysver]['branch'],
        'IP_BSDVERSION'	=> @$remotever[$sysver]['bsdversion'],
        'IP_PLATFORM'	=> $tplatform
       );
 }
 elseif (@isset($_GET['cat']))
 {
  // services list
  $class_services = 'normal';

  // variables
  $cat = trim($_GET['cat']);
  $catlongname = htmlentities(@$db['categories'][$cat]['longname']);

  // handle services table
  foreach (@$db['services'] as $service)
   if ($service['cat'] == $cat)
    $table_services[] = @array(
     'SVC_SHORTNAME'	=> $service['shortname'],
     'SVC_LONGNAME'	=> htmlentities($service['longname']),
     'SVC_DESCRIPTION'	=> htmlentities($service['desc'])
    );
 }
 else
 {
  // categories list
  $class_cat = 'normal';

  // handle category table
  foreach (@$db['categories'] as $cat)
   if (strlen($cat['shortname']) > 0)
   {
    // determine servicecount
    $servicecount = 0;
    foreach ($db['services'] as $servicename => $servicedata)
     if ($servicedata['cat'] == $cat['shortname'])
      $servicecount++;
    $table_categories[] = @array(
     'CAT_SHORTNAME'	=> $cat['shortname'],
     'CAT_LONGNAME'	=> $cat['longname'],
     'CAT_SERVICECOUNT'	=> $servicecount,
     'CAT_DESCRIPTION'	=> $cat['desc']
    );
   }
 }

 return @array(
  'PAGE_ACTIVETAB'	=> 'Install',
  'PAGE_TITLE'		=> 'Install',
  'TABLE_CATEGORIES'	=> $table_categories,
  'TABLE_SERVICES'	=> $table_services,
  'TABLE_INFOPAGE'	=> $table_infopage,
  'NAV_CATSHORT'	=> $nav_catshort,
  'NAV_CATLONG'		=> $nav_catlong,
  'NAV_SVCSHORT'	=> $nav_svcshort,
  'NAV_SVCLONG'		=> $nav_svclong,
  'CLASS_NAV_HASCAT'	=> $class_hascat,
  'CLASS_NAV_HASSVC'	=> $class_hassvc,
  'CLASS_CATEGORIES'	=> $class_cat,
  'CLASS_SERVICES'	=> $class_services,
  'CLASS_INFOPAGE'	=> $class_infopage,
  'INFOPAGE_IMAGEURL'	=> $infopage_img,
  'INFOPAGE_DOWNSIZE'	=> $infopage_downsize,
  'INFOPAGE_SYSVER'	=> $sysimg_str,
  'INFOPAGE_PLATFORM'	=> $platform,
  'CLASS_INFOPAGE_IMG'	=> $ip_class_img,
  'CLASS_INSTALLED'	=> $ip_class_installed,
  'CLASS_INSTALLEDUNAVAIL'	=> $ip_class_installedunavail,
  'CLASS_DOWNLOADING'	=> $ip_class_downloading,
  'CLASS_NOTINSTALLED1'	=> $ip_class_notinstalled1,
  'CLASS_NOTINSTALLED2'	=> $ip_class_notinstalled2,
  'CLASS_NOTINSTALLEDUNAVAIL'	=> $ip_class_notinstalledunavail
 );
}

function submit_services_infopage()
{
 global $guru;

 // required library
 activate_library('guru');
 activate_library('service');

 // redirect url
 $url1 = 'services.php?install';

 // service shortname
 $s = sanitize(trim(@$_POST['service_name']), null, $svc, 32);
 if (!$s)
  friendlyerror('invalid service name', $url1);
 $url2 = $url1.'&service='.$svc;

 // call functions
 $db = service_db();
 $slist = service_list();
 $curver = guru_fetch_current_systemversion();
 $platform = guru_getsystemplatform();

 // gather data
 $sysimg_str = @$curver['sysver'];
 $sysimg = @$db['services'][$svc]['sysimg'][$sysimg_str][$platform];

 // scan POST variables
 foreach ($_POST as $name => $value)
  if ($name == 'download_svc')
  {
   // sanity
   if (!@isset($sysimg['version']))
    error('No download available for your system image / platform');

   // service download sources
   $src_http = (strlen(@$sysimg['url']) > 5) ? $sysimg['url'] : false;
   $src_torrent = (strlen(@$sysimg['torrent']) > 5) ? $sysimg['torrent'] : false;

   // determine download method
   $preftor = (@$guru['preferences']['download_method'] == 'torrent') ?
    true : false;
   if ((($preftor) AND ($src_torrent !== false)) OR
       ((!$preftor) AND ($src_http === false) AND ($src_torrent !== false)))
   {
    // torrent download
    activate_library('torrent');
    torrent_download($src_torrent);
    page_feedback('starting download of torrent: '
     .htmlentities($src_torrent));
   }
   elseif ($src_http !== false)
   {
    // http download
    page_feedback('starting direct download from: '
     .htmlentities($src_http));
    // direct download using fetch (runs on background)
    $cmd = '/usr/bin/fetch -o '.$guru['tempdir'].'/ "'.$src_http.'"';
    exec($cmd.' > /dev/null 2>&1 &');
   }
   else
    error('Could not found a valid download source for service '.$svc);
  }
  elseif ($name == 'install_svc')
  {
   // todo: check for availability
   service_install($svc);
  }
  elseif ($name == 'uninstall_svc')
  { 
   service_uninstall($svc);
  }

 // redirect
 redirect_url($url2);
}

?>
