<?php

function jls()
// returns array of network interfaces and processed data
{
 exec('/usr/local/bin/cbsd jls | tail +2', $myjails);

 // first split raw output into chunks (one chunk per interface)
 $chunks = array();
 $myjails_str = '';

    $detailed = array ();

 if (@is_array($myjails))
 {
    foreach ($myjails as $line) {
    $pieces = explode(" ",preg_replace('/\s+/', ' ', $line));

    $action=($pieces[5]=='On')?'<span class="red">Stop</span>':'Start';
    $action_cmd=($pieces[5]=='On')?'stop':'start';
    
  // add interface to detailed array
  $detailed[$pieces[0]] = array(
   'ident'			=> "localhost",
   'jname'			=> $pieces[0],
   'jid'				=> $pieces[1],
   'ip'					=> $pieces[2],
   'fqdn'				=> $pieces[3],
   'path'				=> $pieces[4],
   'status'			=> $pieces[5],
   'action'			=> $action,
   'action_cmd'			=> $action_cmd,
  );
 }
} //foreach
 // return detailed array
 return $detailed;
}


?>
