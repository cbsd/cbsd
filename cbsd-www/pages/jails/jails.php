<?php

function content_jails_jails()
{
 // required library
 activate_library('jails');
 // call function
 $interfaces = jls();

 // queried interface
 $queryj = @$_GET['query'];

 // process table JLIST
 $JLIST = array();
 foreach ($interfaces as $ifname => $jdata)
 {
  // activerow
  if ((strlen($ifname) > 0) AND ($ifname == $queryj))
   $activerow = 'class="activerow"';
  else
   $activerow = '';

  // ident
  $ident_maxlen = 50;
  if (@strlen($jdata['ident']) > $ident_maxlen)
   $ident = '<acronym title="'.htmlentities($jdata['ident']).'">'
    .substr(htmlentities($jdata['ident']), 0, $ident_maxlen).'..</acronym>';
  else
   $ident = htmlentities($jdata['ident']);
   
  $JLIST[] = array(
   'JLIST_ACTIVEROW'	=> $activerow,
   'JLIST_JNAME'	=> $jdata['jname'],
   'JLIST_JID'	=> $jdata['jid'],
   'JLIST_IP'		=> $jdata['ip'],
   'JLIST_FQDN'	=> $jdata['fqdn'],
   'JLIST_PATH'		=> $jdata['path'],
   'JLIST_STATUS'	=> $jdata['status'],
   'JLIST_ACTION'	=> $jdata['action'],
   'JLIST_ACTION_CMD'	=> $jdata['action_cmd'],
  );
 }

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'		=> 'Jails',
  'PAGE_TITLE'			=> 'Jails',
  'TABLE_NETWORK_JLIST'	=> $JLIST
 );
 return $newtags;
}

?>
