<?php

function content_system_activation()
{
 global $guru, $tabs;

 // required library
 activate_library('activation');
 activate_library('persistent');

 // add activation tab
 $tabs['Activation'] = 'system.php?activation';

 // uuid
 $uuid = @$guru['preferences']['uuid'];
 $uuid_txt = (strlen($uuid) > 0) ? $uuid : '-';

 // server status
 $host = 'activation.zfsguru.com';
 $aliveurl = '/zfsguru_alive.txt';
 $guru['no_delayed_activation'] = true;
 $online = (stripos(remoteserver($aliveurl, $host), 'online') !== false);

 // delayed activation
 if ($online)
 {
  unset($guru['no_delayed_activation']);
  activation_delayed();
 }

 // activation details
 $info = ($online) ? activation_info() : false;

 // dmesg file
 $dmesg = file_get_contents('/var/run/dmesg.boot');
 $dpos = strrpos($dmesg, 'Copyright (c) 1992-2011 The FreeBSD Project.');
 $dmesg = substr($dmesg, (int)$dpos);

 // check for changed hardware
 $hwchanged = false;
 if (strlen($uuid) > 0)
  $hwchanged = activation_hwchange();

 // late activation data
 $delayed = persistent_read('activation_delayed');
 $lateactdata = (@strlen($delayed['activation_type']) > 0) ? true : false;

 // classes
 $class_activated = (strlen($uuid) > 0) ? 'normal' : 'hidden';
 $class_activation4 = (@$info['type'] == 2) ? 'normal' : 'hidden';
 $class_activation5 = ($hwchanged) ? 'normal' : 'hidden';
 $class_notactivated = (strlen($uuid) > 0) ? 'hidden' : 'normal';
 $class_online = ($online) ? 'normal' : 'hidden';
 $class_offline = ($online) ? 'hidden' : 'normal';
 $class_datasent = (@isset($_GET['datasent'])) ? 'normal' : 'hidden';
 $class_lateact = ($lateactdata) ? 'normal' : 'hidden';

 // restore activation data from delayed activation or remote info
 if (@strlen($delayed['activation_type']) > 0)
  $type = array((int)$delayed['activation_type'] => 'checked="checked"');
 elseif (@$info['type'] == 2)
  $type = array(2 => 'checked="checked"');
 else
  $type = array(1 => 'checked="checked"');
 if (@strlen($delayed['early_feedback']) > 0)
  $feedback = array((int)$delayed['early_feedback'] => 'checked="checked"');
 elseif (@is_int($info['feedback']))
  $feedback = array((int)$info['feedback'] => 'checked="checked"');
 else
  $feedback = array(1 => 'checked="checked"');
 if (strlen(@$delayed['feedback_text']) > 0)
  $feedback_txt = @$delayed['feedback_text'];
 elseif (strlen(@$info['feedback_text']) > 0)
  $feedback_txt = @$info['feedback_text'];
 else
  $feedback_txt = '';

 // export new tags
 return @array(
  'PAGE_TITLE'		=> 'Activation',
  'CLASS_ACTIVATED'	=> $class_activated,
  'CLASS_ACTIVATION4'	=> $class_activation4,
  'CLASS_ACTIVATION5'	=> $class_activation5,
  'CLASS_NOTACTIVATED'	=> $class_notactivated,
  'CLASS_ONLINE'	=> $class_online,
  'CLASS_OFFLINE'	=> $class_offline,
  'CLASS_DATASENT'	=> $class_datasent,
  'CLASS_LATEACT'	=> $class_lateact,
  'ACT_UUID'		=> $uuid_txt,
  'ACT_DMESG'		=> htmlentities($dmesg),
  'ACT_TYPE_1'		=> $type[1],
  'ACT_TYPE_2'		=> $type[2],
  'ACT_TYPE_3'		=> $type[3],
  'ACT_FEEDBACK_1'	=> $feedback[1],
  'ACT_FEEDBACK_2'	=> $feedback[2],
  'ACT_FEEDBACK_3'	=> $feedback[3],
  'ACT_FEEDBACK_4'	=> $feedback[4],
  'ACT_FEEDBACK_TXT'	=> $feedback_txt
 );
}

function submit_activate()
{
 global $guru;

 if (@isset($_POST['nuke_lateact']))
 {
  // destroy late activation data
  activate_library('persistent');
  persistent_remove('activation_delayed');
  page_feedback('delayed activation data removed!', 'c_notice');
 }
 elseif (@isset($_POST['activation4']))
 {
  // level 4 activation (upgrade from type A2 to A1)
  activate_library('activation');
  // activate
  $uuid = @activation_submit(4, (int)$_POST['feedback'],
   $_POST['feedback_text']);
  if (strlen($uuid) > 0)
  {
   page_feedback('activation was successful!', 'b_success');
   // save uuid
   $guru['preferences']['uuid'] = $uuid;
   procedure_writepreferences($guru['preferences']);
  }
  else
   page_feedback('could not activate at this time - trying again later '
    .'using the data you supplied', 'a_failure');
 }
 elseif (@isset($_POST['activation5']))
 {
  // level 5 activation (A1 activation with changed hardware)
  activate_library('activation');
  // activate
  $uuid = @activation_submit(5, (int)$_POST['feedback'],
   $_POST['feedback_text']);
  if (strlen($uuid) > 0)
  {
   page_feedback('activation was successful!', 'b_success');
   // save uuid
   $guru['preferences']['uuid'] = $uuid;
   procedure_writepreferences($guru['preferences']);
  }
  else
   page_feedback('could not activate at this time - trying again later '
    .'using the data you supplied', 'a_failure');
 }
 elseif (@isset($_POST['activation_update']))
 {
  // required library
  activate_library('activation');
  // activate
  $uuid = @activation_submit((int)$_POST['activation'], (int)$_POST['feedback'],
   $_POST['feedback_text']);
  if (strlen($uuid) > 0)
  {
   page_feedback('activation was successful!', 'b_success');
   // save uuid
   $guru['preferences']['uuid'] = $uuid;
   procedure_writepreferences($guru['preferences']);
  }
  else
   page_feedback('could not activate at this time - trying again later '
    .'using the data you supplied', 'a_failure');
 }

 // redirect
 redirect_url('system.php?activation');
}

function submit_activate_reset()
{
 global $guru;
 $guru['preferences']['uuid'] = '';
 procedure_writepreferences($guru['preferences']);
 redirect_url('system.php?activation');
}

?>
