<?php

function content_internal_dangerouscommand($data)
{
 // command array
 if (@is_string($data['commands']))
  $command_arr = array($data['commands']);
 elseif (@is_array($data['commands']))
  $command_arr = $data['commands'];
 else
  $command_arr = array();

 // command string
 $command_str = @implode(chr(10), $command_arr);

 // command list
 $commandlist = array();
 foreach ($command_arr as $id => $command)
  $commandlist[] = array(
   'CMD_ID'	=> htmlentities($id),
   'CMD'	=> htmlentities($command)
  );

 // command count
 $commandcount = count($command_arr);

 // redirect URL
 $redirect_url = @$_POST['redirect_url'];

 // export new tags
 $newtags = array(
  'PAGE_TITLE'		=> 'Dangerous command execution',
  'TABLE_COMMANDLIST'	=> $commandlist,
  'COMMAND_STR'		=> $command_str,
  'COMMAND_COUNT'	=> $commandcount,
  'REDIRECT_URL'	=> @$data['redirect_url']
 );
 return $newtags;
}

?>
