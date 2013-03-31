<?php

/* privileged super user functions */

function super_execute($command, $raw_output = false)
// elevated privileges command execution
{
 if ($raw_output === false)
 {
  // TODO - SECURITY
  exec('/usr/local/bin/sudo '.$command.' 2>&1', $result, $rv);
  $result_str = implode(chr(10), $result);
  return array(
   'rv'		=> $rv,
   'output_arr'	=> $result,
   'output_str'	=> $result_str
  );
 }
 else
 {
  // raw output
  system('/usr/local/bin/sudo '.$command.' 2>&1', $rv);
  return $rv;
 }
}

function super_script($script_name, $parameters = '')
// execute ZFSguru script at elevated privileges
{
 global $guru;
 if (@strlen($script_name) < 1)
  error('HARD ERROR: no script name!');
 $command = '/scripts/'.$script_name.'.sh '.$parameters;
 $result = super_execute($guru['docroot'].$command);
 return $result;
}

?>
