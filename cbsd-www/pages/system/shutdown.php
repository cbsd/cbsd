<?php

function submit_system_shutdown()
{
 // elevated privileges
 activate_library('super');

 // set active tab
 page_injecttag(array('PAGE_ACTIVETAB' => 'Shutdown'));

 if (@isset($_POST['reboot_system']))
 {
  // display shutdown page
  page_injecttag(array('PAGE_TITLE' => 'REBOOTING'));
  $content = '<h1>REBOOTING SYSTEM!</h1>'
   .'<p>The server is currently rebooting. '
   .'Wait for a moment then try to reconnect.</p>'
   .'<p><a href="status.php">Click here to try to reconnect</a><p>';
  page_handle($content);
  flush();

  // execute command
  $result = super_execute('/sbin/shutdown -r now');
  if (@$result['rv'] != 0)
   error('Could not execute system reboot');
 }
 elseif (@isset($_POST['powerdown_system']))
 {
  // display shutdown page
  page_injecttag(array('PAGE_TITLE' => 'POWERDOWN'));
  $content = '<h1>SHUTDOWN INITIATED!</h1>'
   .'<p>The server has initiated a shutdown.</p>'
   .'<p><a href="status.php">Click here to try to reconnect</a><p>';
  page_handle($content);
  flush();

  // execute command
  $result = super_execute('/sbin/shutdown -p now');
  if (@$result['rv'] != 0)
   error('Could not execute system shutdown');
 }
 else
  error('Unhandled shutdown function');

 // stop execution
 die();
}

?>
