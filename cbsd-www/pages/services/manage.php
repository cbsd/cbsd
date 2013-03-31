<?php

function content_services_manage()
{
 // required library
 activate_library('service');

 // gather data
 $services = service_list();
 $db = service_db();

 // queried service
 $query = @$_GET['query'];

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
  $longname = (@strlen($db['services'][$service]['longname']) > 0) ?
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

 // export new tags
 return array(
  'PAGE_ACTIVETAB'	=> 'Manage',
  'PAGE_TITLE'		=> 'Manage',
  'TABLE_SERVICELIST'	=> @$servicelist,
  'CLASS_SERVICES'	=> $class_services,
  'CLASS_NOSERVICES'	=> $class_noservices
 );
}

function submit_services_manage()
{
 global $guru;

 // required library
 activate_library('service');

 // redirect url
 $url = 'services.php?manage';

 foreach ($_POST as $name => $value)
 {

  if (substr($name, 0, strlen('svc_start_')) == 'svc_start_')
  {
   // start service
   $svc = trim(substr($name, strlen('svc_start_')));
   $result = service_start($svc);
   if ($result)
    friendlynotice('service '.htmlentities($svc).' started!', $url);
   else
    friendlyerror('could not start service '.htmlentities($svc).'!', $url);
  }

  if (substr($name, 0, strlen('svc_stop_')) == 'svc_stop_')
  {
   // stop service
   $svc = trim(substr($name, strlen('svc_stop_')));
   $result = service_stop($svc);
   if ($result)
    friendlynotice('service '.htmlentities($svc).' stopped!', $url);
   else
    friendlyerror('could not stop service '.htmlentities($svc).'!', $url);
  }

 }
}

?>
