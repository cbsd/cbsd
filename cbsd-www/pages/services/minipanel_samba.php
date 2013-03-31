<?php

function content_services_minipanel_samba()
{
 // required modules
 activate_library('samba');

 // read samba configuration
 $sambaconf = samba_readconfig();
 if ($sambaconf === false)
  error('Could not read Samba configuration file!');

 // workgroup
 $workgroup = htmlentities($sambaconf['global']['workgroup']);

 // security model
 $sm_share = ($sambaconf['global']['security'] == 'share') 
  ? 'selected="selected"' : '';
 $sm_domain = ($sambaconf['global']['security'] == 'domain')
  ? 'selected="selected"' : '';
 $sm_ads = ($sambaconf['global']['security'] == 'ads')
  ? 'selected="selected"' : '';
 $sm_server = ($sambaconf['global']['security'] == 'server')
  ? 'selected="selected"' : '';

 // authentication backend
 $ab_ldapsam = ($sambaconf['global']['passdb backend'] == 'ldapsam')
  ? 'selected="selected"' : '';
 $ab_smbpasswd = ($sambaconf['global']['passwd backend'] == 'smbpasswd')
  ? 'selected="selected"' : '';

 // other classes
 $class_corruptconfig = ($sambaconf === false) ? 'normal' : 'hidden';
 $class_noshares = (@count($sambaconf['shares']) < 1) ? 'normal' : 'hidden';
 $class_deleteselectedshares = ($class_noshares == 'hidden') 
  ? '' : 'disabled="disabled"';

 // table: extraglobals
 $table_extraglobals = array();
 $globalvars = array('workgroup', 'security', 'passdb backend');
 foreach ($sambaconf['global'] as $property => $value)
  if (!in_array($property, $globalvars))
   $table_extraglobals[] = array(
    'EXTRAGLOB_PROPERTY' => $property,
    'EXTRAGLOB_VALUE' => $value
   );

 // table: shares
 $table_shares = array();
 if (@is_array($sambaconf['shares']))
  foreach ($sambaconf['shares'] as $sharename => $share)
  {
   // set some checkboxes on/off according to data
   // default if non present:
   // browseable = yes (synonym: browsable)
   // writeable = no (synonym: writable)
   // public = no (synonym: guest ok)
   $nobrowse = '';
   if (@$share['browsable'] == 'no')
    $nobrowse = 'selected="on"';
   if (@$share['browseable'] == 'no')
    $nobrowse = 'selected="on"';
   $nowrite = 'selected="on"';
   if (@$share['writable'] == 'yes')
    $nowrite = '';
   if (@$share['writeable'] == 'yes')
    $nowrite = '';
   $notpublic = 'selected="on"';
   if (@$share['public'] == 'yes')
    $notpublic = '';
   if (@$share['guest ok'] == 'yes')
    $notpublic = '';

   // extra share options
   $share_extra = '';
   foreach ($share as $name => $value)
    if (!in_array(trim($name), array('path', 'comment', 'browseable',
     'writeable', 'public')))
     $share_extra .= '<tr><td>'.htmlentities($name).'</td>'
      .'<td>'.@htmlentities($value).'</td></tr>';

   // add row to table
   $table_shares[] = array(
    'SHARE_NAME'	=> htmlentities($sharename),
    'SHARE_PATH'	=> $share['path'],
    'SHARE_COMMENT'	=> $share['comment'],
    'SHARE_NOBROWSE'	=> $nobrowse,
    'SHARE_NOWRITE'	=> $nowrite,
    'SHARE_NOTPUBLIC'	=> $notpublic,
    'SHARE_EXTRA'	=> $share_extra
   );
  }

 // export new tags
 return array(
  'TABLE_SAMBA_EXTRAGLOBALS'	=> $table_extraglobals,
  'TABLE_SAMBA_SHARES'		=> $table_shares,
  'CLASS_SAMBA_CORRUPTCONFIG'	=> $class_corruptconfig,
  'CLASS_SM_SHARE'		=> $sm_share,
  'CLASS_SM_DOMAIN'		=> $sm_domain,
  'CLASS_SM_ADS'		=> $sm_ads,
  'CLASS_SM_SERVER'		=> $sm_server,
  'CLASS_AB_LDAPSAM' 		=> $ab_ldapsam,
  'CLASS_AB_SMBPASSWD'		=> $ab_smbpasswd,
  'CLASS_SAMBA_NOSHARES'	=> $class_noshares,
  'CLASS_DELETESELECTEDSHARES'	=> $class_deleteselectedshares,
  'SAMBA_WORKGROUP'		=> $workgroup
 );
}

function submit_minipanel_samba()
{
 // required modules
 activate_library('samba');

 // read samba configuration
 $sambaconf = samba_readconfig();
 if ($sambaconf === false)
  error('Could not read Samba configuration file!');

 // redirect URL
 $redir = 'services.php?internal&query=samba';

 // check for submitted form
 if (@isset($_POST['samba_delete_shares']))
 {
  // only remove shares and write changes to disk
  $newconf = $sambaconf;
  foreach ($_POST as $name => $value)
   if (substr($name,0,strlen('deleteshare-')) == 'deleteshare-')
    unset($newconf['shares'][trim(substr($name,strlen('deleteshare-')))]);
  // save configuration
  $result = samba_writeconfig($newconf);
  // redirect
  if ($result !== true)
   error('Error writing Samba configuration file ("'.$result.'")');
  else
   friendlynotice('One or more Samba shares deleted!', $redir);
 }
 elseif (@isset($_POST['samba_update_config']))
 {
  // update samba configuration with user submitted changes
  $newconf = $sambaconf;
  // process global variables
  foreach ($_POST as $name => $value)
   if (substr($name,0,strlen('global-')) == 'global-')
    $newconf['global'][trim(substr($name,strlen('global-')))] = trim($value);
  // process shares
  foreach ($newconf['shares'] as $sharename => $share)
  {
   // todo: create share property if not yet existent
   // and remove if unset
   $check = array('path', 'comment', 'browseable', 'writeable', 'public');
   // make sure variables exist to begin with, or unset them if needed
   foreach ($check as $checkvar)
   {
    if ((!@isset($share[$checkvar])) AND
     (strlen($_POST['share'.$sharename.'-'.$checkvar]) > 0))
     $share[$checkvar] = '';
    if ((@isset($share[$checkvar])) AND
     (@strlen($_POST['share'.$sharename.'-'.$checkvar]) < 1))
     unset($newconf['shares'][$sharename][$checkvar]);
   }
   // now run through the $share array and update any variables applicable
   foreach ($share as $sharevariable => $value)
    if (@strlen($_POST['share'.$sharename.'-'.$sharevariable]) > 0)
     $newconf['shares'][$sharename][$sharevariable] =
      $_POST['share'.$sharename.'-'.$sharevariable];
  }
  // save configuration
  $result = samba_writeconfig($newconf);
  // redirect
  if ($result !== true)
   error('Error writing Samba configuration file ("'.$result.'")');
  else
   friendlynotice('Samba configuration updated!', $redir);
 }
 elseif (@isset($_POST['samba_restart_samba']))
 {
  $result = samba_restartservice();
  if ($result == 0)
   friendlynotice('Samba service restarted!', $redir);
  else
   friendlyerror('Could not restart Samba ('.$result.')', $redir);
 }
 // no catch redirect
 redirect_url($redir);
}

?>
