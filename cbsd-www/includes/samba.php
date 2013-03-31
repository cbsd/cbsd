<?php

function samba_readconfig()
// reads samba configuration file, and returns a complex array
{
 global $guru;
 // fetch configuration file contents
 $rawtext = @file_get_contents($guru['path']['Samba']);
 if (strlen($rawtext) < 1)
  return false;
 else
 {
  // begin by splitting the configuration file in three chunks
  $split = preg_split('/^\#(\=)+(.*)(\=)+\r?$/m', $rawtext);
  // now fetch all the global variables
  preg_match_all('/^[ ]*([a-zA-Z0-9]+(\s[a-zA-Z0-9]+)*)[ ]*\=[ ]*(.*)$/Um', 
   $split[1], $global);
  if (is_array($global))
   foreach ($global[1] as $id => $propertyname)
    $config['global'][trim($propertyname)] = trim($global[3][$id]);
  else
   return false;

  // now work on the shares
  $sharesplit = preg_split('/^\[([a-zA-Z0-9]+)\]/Um', $split[2]);
  preg_match_all('/^\[([a-zA-Z0-9]+)\]/Um', $split[2], $sharenames_match);
  $sharenames = $sharenames_match[1];
  // the following did not work, due to PREG_SPLIT_DELIM_CAPTURE messing
  // with the results. unknown issue; to be looked at?
  // circumvented with separate sharename regexp
//  $sharesplit = preg_split('/^\[([a-zA-Z0-9]+)\]/Um', $split[2], 
//   PREG_SPLIT_DELIM_CAPTURE);
//  $sharesplit = preg_split('/^\[([a-zA-Z0-9]+)\]\r?$/Um', $split[2]);
  if (is_array($sharesplit))
   foreach ($sharesplit as $sid => $singleshare)
   {
    preg_match_all('/^[ ]*([a-zA-Z0-9]+(\s[a-zA-Z0-9]+)*)[ ]*\=[ ]*(.*)$/m', 
     $singleshare, $sharecontents);
    $sharename = @$sharenames[$sid-1];
    // add the shares to the config array, to be returned by this function
    if (is_array($sharecontents) AND (@strlen($sharename) > 0))
     foreach ($sharecontents[1] as $id => $propertyname)
      $config['shares'][trim($sharename)][trim($propertyname)] =
       $sharecontents[3][$id];
   }
 }
 return $config;
}

function samba_writeconfig($newconfig)
// writes updated samba configuration file to disk
{
 global $guru;

 // elevated privileges
 activate_library('super');

 if (!is_array($newconfig))
  error('Invalid call to function samba_writeconfig()');
 // fetch configuration file contents
 $rawtext = @file_get_contents($guru['path']['Samba']);
 if (strlen($rawtext) < 1)
  return false;
 // now do all the heavy work of intepreting the configuration file
 $split = preg_split('/^\#(\=)+(.*)(\=)+\r?$/m', $rawtext);
 // check for expected format
 if ((count($split) != 3) OR (!is_string(@$split[1])))
  error('Samba configuration file smb.conf differs from expected format.');

 // start work on the globals section
 foreach ($newconfig['global'] as $name => $value)
  $split[1] = preg_replace('/^[ ]*('.$name.')[ ]*\=[ ]*(.*)$/Um',
   trim($name).' = '.trim($value), $split[1], 1);

 // start work on the shares section
 $shareblock = chr(10);
 foreach ($newconfig['shares'] as $sharename => $share)
 {
  $shareblock .= chr(10).'['.$sharename.']'.chr(10);
  foreach ($share as $shareproperty => $propertyvalue)
   $shareblock .= trim($shareproperty).' = '.trim($propertyvalue).chr(10);
 }

 // now glue split parts together again
 $combined =
  $split[0]
  .'#======================= Global Settings '
  .'====================================='
  .$split[1]
  .'#============================ Share Definitions '
  .'=============================='
  .$shareblock;
 // before we overwrite new configuration file, set permissions for php access
 super_execute('/bin/chmod 666 '.$guru['path']['Samba']);
 // now write the new configuration file to disk
 $result = file_put_contents($guru['path']['Samba'], $combined);
 // and reset the permissions again
 super_execute('/bin/chmod 444 '.$guru['path']['Samba']);
 return true;
}

function samba_restartservice()
// restarts samba after a configuration change
{
 global $guru;

 // elevated privileges (not necessary?!!?!)
 activate_library('super');

 // restart samba
 $result = super_execute($guru['rc.d']['Samba'].' restart');
 return ($result['rv']);
}

function samba_isshared($path)
// returns share name if given pathname is shared, false if not
{
 $config = samba_readconfig();
 if (@!is_array($config['shares']))
  return false;
 foreach ($config['shares'] as $sharename => $share)
  if ($share['path'] == trim($path))
   return $sharename;
 return false;
}

function samba_removesharepath($mountpoint)
// search for any shares with given path and remove them from configuration
{
 // read samba configuration
 $sambaconf = samba_readconfig();
 // locate share with given mountpoint
 foreach ($sambaconf['shares'] as $sharename => $sharedata)
  if ($sharedata['path'] == $mountpoint)
  {
   // remove share from array
   unset($sambaconf['shares'][$sharename]);
   // write new configuration and return true
   samba_writeconfig($sambaconf);
   return true;
  }
 // nothing done; return false
 return false;
}

function samba_removesharepath_recursive($mountpoint)
// removes any shares with a path starting with supplied mountpoint (recursive)
{
 // read samba configuration
 $sambaconf = samba_readconfig();
 // locate share with mountpoint that begins with supplied mountpoint
 $changed = false;
 foreach ($sambaconf['shares'] as $sharename => $sharedata)
  if (substr($sharedata['path'], 0, strlen($mountpoint)) == $mountpoint)
  {
   // remove share from array
   unset($sambaconf['shares'][$sharename]);
   $changed = true;
  }
 if ($changed)
 {
  // write new configuration and return true
  samba_writeconfig($sambaconf);
  return true;
 }
 else
  return false;
}

?>
