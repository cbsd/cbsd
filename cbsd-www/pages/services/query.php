<?php

function content_services_query()
{
 global $guru;

 // required library
 activate_library('service');

 // gather data
 $services = service_list();
 $db = service_db();

 // queried service
 $query = @$_GET['query'];

 // redirect on bad service
 if (strlen($query) > 0)
  if (!@isset($services[$query]))
   friendlyerror('this service does not exist', 'services.php?manage');

 // servicelist table
 $servicelist = array();
 foreach ($services as $service => $data)
 {
  $activerow = ($query == $service) ? 'activerow' : 'normal';
  if ($data['status'] == 'passive')
  {
   $status = 'PASSIVE';
   $class_status = 'grey';
  }
  elseif ($data['status'] == 'running')
  {
   $status = 'RUNNING';
   $class_status = 'green';
  }
  elseif ($data['status'] == 'stopped')
  {
   $status = 'STOPPED';
   $class_status = 'red';
  }
  else
  {
   $status = @htmlentities(strtoupper($data['status']));
   $class_status = 'blue';
  }

  // classes
  $class_startbutton = @(($data['status'] != 'running') AND $data['can_start'])
   ? 'normal' : 'hidden';
  $class_stopbutton = @(($data['status'] == 'running') AND $data['can_stop'])
   ? 'normal' : 'hidden';

  // autostart
  $autostart = '-';

  // long name (if unknown use shortname instead)
  $longname = (@$db['services'][$service]['longname']) ? 
   htmlentities($db['services'][$service]['longname']) : $service;

  // long cat (if unknown use shortcat instead)
  $longcat = (@strlen($db['categories'][$data['cat']]['longname']) > 0) ?
   htmlentities($db['categories'][$data['cat']]['longname']) : $data['cat'];

  // add row to servicelist table
  $servicelist[] = @array(
   'CLASS_ACTIVEROW'	=> $activerow,
   'SERVICE_NAME'	=> htmlentities($service),
   'SERVICE_LONGNAME'	=> $longname,
   'SERVICE_CAT'	=> $longcat,
   'SERVICE_VER_EXT'	=> $data['ver_ext'],
   'SERVICE_VER_PROD'	=> $data['ver_prod'],
   'SERVICE_SIZE'	=> sizebinary($data['size'], 1),
   'CLASS_STATUS'	=> $class_status,
   'SERVICE_STATUS'	=> $status,
   'CLASS_STOPBUTTON'	=> $class_stopbutton,
   'CLASS_STARTBUTTON'	=> $class_startbutton,
   'SERVICE_AUTOSTART'	=> $autostart
  );
 }

 // hide noservices div when services are present
 $class_services = (@empty($services)) ? 'hidden' : 'normal';
 $class_noservices = (@empty($services)) ? 'normal' : 'hidden';

 // define service
 $service = @$services[$query];

 // long name (if unknown use shortname instead)
 $longname = (@$db['services'][$query]['longname']) ?
  htmlentities($db['services'][$query]['longname']) : $query;

 // longcat
 $longcat = (@strlen($db['categories'][$service['cat']]['longname']) > 0) ?
   htmlentities($db['categories'][$service['cat']]['longname']) : 
    $service['cat'];

 // service path
 $qpath = $guru['path_services'].'/'.trim($query);

 // export new tags
 return @array(
  'PAGE_ACTIVETAB'	=> 'Manage',
  'PAGE_TITLE'		=> 'Manage',
  'TABLE_SERVICELIST'	=> @$servicelist,
  'CLASS_SERVICES'	=> $class_services,
  'CLASS_NOSERVICES'	=> $class_noservices,
  'QSERVICE'		=> htmlentities($query),
  'QSERVICE_LONG'	=> $longname,
  'QSERVICE_PATH'	=> $qpath,
  'QSERVICE_CAT'	=> $longcat,
  'QSERVICE_VER_EXT'	=> $service['ver_ext'],
  'QSERVICE_VER_PROD'	=> $service['ver_prod'],
  'QSERVICE_SYSVER'	=> $service['sysver'],
  'QSERVICE_PLATFORM'	=> $service['platform'],
  'QSERVICE_SIZE'	=> sizebinary($service['size'], 1),
  'QSERVICE_LICENSE'	=> $service['license'],
  'QSERVICE_DEPEND'	=> $service['depend'],
  'QSERVICE_SECURITY'	=> $service['security'],
  'QSERVICE_CANSTART'	=> $service['can_start']
 );
}

?>
