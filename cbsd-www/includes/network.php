<?php

function network_interfaces()
// returns array of network interfaces and processed data
{
 // fetch dmesg.boot
 $dmesg = file_get_contents('/var/run/dmesg.boot');

 // fetch ifconfig raw output
 exec('/sbin/ifconfig', $ifconfig);

 // first split raw output into chunks (one chunk per interface)
 $chunks = array();
 $ifconfig_str = '';
 if (@is_array($ifconfig))
 {
  foreach ($ifconfig as $line)
   $ifconfig_str .= $line.chr(10);
  $arr = preg_split('/^([a-zA-Z0-9]*)\: /m', $ifconfig_str, 
   null, PREG_SPLIT_NO_EMPTY + PREG_SPLIT_DELIM_CAPTURE);
  // for every even array ID we process two array IDs at once
  foreach ($arr as $id => $chunk)
   if (!((int)$id & 1) AND (strlen($chunk) <= 8))
   {
    $if_name = trim($chunk);
    $chunks[trim($chunk)] = trim($arr[(int)$id+1]);
   }
 }

 // process chunks into detailed arrays
 $detailed = array();
 foreach ($chunks as $ifname => $ifdata)
 {
  // process flags= line
  $preg1 = '/^flags\=([0-9]+)\<([a-zA-Z0-9,_]+)\> metric ([0-9]+) mtu ([0-9]+)/';
  preg_match($preg1, $ifdata, $matches);
  $flags = @$matches[1];
  $flags_str = @$matches[2];
  $metric = @$matches[3];
  $mtu = @$matches[4];
  
  // process options= line
  $preg2 = '/^[\s]*options\=([a-f0-9]+)\<([a-zA-Z0-9,_]+)\>/m';
  preg_match($preg2, $ifdata, $matches2);
  $options = @$matches2[1];
  $options_str = @$matches2[2];
 
  // process ether line
  $preg3 = '/^[\s]*ether (([a-f0-9]{2}\:){5}[a-f0-9]{2})/m';
  preg_match($preg3, $ifdata, $matches3);
  $ether = @$matches3[1];

  // process inet lines
  $preg4 = '/^[\s]*inet (([0-9]{1,3}\.){3}[0-9]{1,3}) '
   .'netmask (0x[0-9a-f]{8})( broadcast (([0-9]{1,3}\.){3}[0-9]{1,3}))?/m';
  preg_match_all($preg4, $ifdata, $matches4);

  // construct inet array
  $inet = array();
  if (is_array($matches4[1]))
   foreach ($matches4[1] as $id => $ipaddress)
    $inet[] = array(
     'ip'		=> $ipaddress,
     'netmask'		=> @$matches4[3][$id],
     'broadcast'	=> @$matches4[5][$id]
    );

  // process inet6 lines
  $preg5 = '/^[\s]*inet6 ([a-z0-9:%]+) prefixlen ([0-9]+)( scopeid (.*))?/m';
  preg_match_all($preg5, $ifdata, $matches5);

  // construct inet6 array
  $inet6 = array();
  if (is_array($matches5[1]))
   foreach ($matches5[1] as $id => $ipaddress)
    $inet6[] = array(
     'ip'		=> $ipaddress,
     'prefixlen'	=> @$matches5[2][$id],
     'scopeid'		=> @trim($matches5[4][$id])
    );

  // determine IP address based on either inet or inet6 configuration
  $ip = '';
  foreach ($inet6 as $id => $inetdata)
   if (@strlen($inetdata['ip']) > 0)
    $ip = $inetdata['ip'];
  foreach ($inet as $id => $inetdata)
   if (@strlen($inetdata['ip']) > 0)
    $ip = $inetdata['ip'];

  // process media line
  $preg6 = '/^[\s]*media\: ([^()]+) \(([^)]+)\)/m';
  preg_match($preg6, $ifdata, $matches6);
  $media = @$matches6[1];
  $linkspeed = @$matches6[2];
  // TODO: duplex (fetch from linkspeed)
  $duplex = '';

  // process status line
  $preg7 = '/^[\s]*status\: (.*)/m';
  preg_match($preg7, $ifdata, $matches7);
  $status = @$matches7[1];

  // add dmesg ident string
  $preg8 = '/^'.$ifname.'\: \<(.+)\>/m';
  preg_match($preg8, $dmesg, $matches8);
  $ident = @$matches8[1];

  // add interface to detailed array
  $detailed[$ifname] = array(
   'ifname'		=> $ifname,
   'ident'		=> $ident,
   'flags'		=> $flags,
   'flags_str'		=> $flags_str,
   'metric'		=> $metric,
   'mtu'		=> $mtu,
   'options'		=> $options,
   'options_str'	=> $options_str,
   'ether'		=> $ether,
   'inet'		=> $inet,
   'inet6'		=> $inet6,
   'ip'			=> $ip,
   'media'		=> $media,
   'linkspeed'		=> $linkspeed,
   'duplex'		=> $duplex,
   'status'		=> $status
  );
 }

 // return detailed array
 return $detailed;
}

?>
