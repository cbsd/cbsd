<?php

function content_status_status()
{
 global $guru;

 // required libraries
 activate_library('disk');
 activate_library('guru');

 // date of last modified script in webinterface directory
 $script_date = '';
 @exec('/usr/bin/find '.$guru['docroot'].' -name "*.php"', $files_php);
 @exec('/usr/bin/find '.$guru['docroot'].' -name "*.css"', $files_css);
 foreach (@$files_php as $fname)
  if (@filemtime($fname) > $script_date)
   $script_date = filemtime($fname);
 foreach (@$files_css as $fname)
  if (@filemtime($fname) > $script_date)
   $script_date = filemtime($fname);
 date_default_timezone_set('UTC');
 if ((int)$script_date > 0)
  $script_date = @date($guru['iso_date_format'], $script_date).' UTC';
 date_default_timezone_set($guru['preferences']['timezone']);

 // version data
 $fbsdver = trim(`uname -r`);
 $currentver = guru_fetch_current_systemversion();
 $syszfsver = guru_zfsversion();
 $zfs_spa = $syszfsver['spa'];
 $zfs_zpl = $syszfsver['zpl'];

 // cpu and memory
 $cpu = guru_sysctl('hw.model');
 $arch = trim(`uname -p`);
 if ($arch == 'amd64')
  $arch = 'amd64 (64-bit)';
 elseif ($arch == 'i386')
  $arch = 'i386 (32-bit)';
 $mem = `sysctl -n hw.physmem`;
 $mem_human = sizebinary((int)$mem, 1);
 $kmem = `sysctl -n vm.kmem_size`;
 $kmem_human = sizebinary((int)$kmem, 1);
 $kmem_max = `sysctl -n vm.kmem_size_max`;
 $kmem_max_human = sizebinary((int)$kmem_max, 1);

 // system data
 $systime = trim(`date`);
 $cmd = trim(`uptime`);
 $tmp = substr($cmd, strpos($cmd, 'up ')+3);
 $uptime = substr($tmp, 0, strpos($tmp,','));
 $loadavg = trim(substr($cmd, strrpos($cmd, ':')+1));
 $tmp = substr($loadavg, 0, strpos($loadavg, ','));
 $cpuusage = ((double)$tmp * 100) . ' %';
 $disks = disk_detect_physical();
 $diskcount = count($disks);

 // sensors
 if (strtolower(substr($cpu, 0, strlen('AMD'))) == 'amd')
 {
  exec('/sbin/kldstat -n amdtemp.ko', $output, $rv);
  if ($rv == 1)
   guru_loadkernelmodule('amdtemp');
 }
 elseif (strtolower(substr($cpu, 0, strlen('Intel'))) == 'intel')
 {
  exec('/sbin/kldstat -n coretemp.ko', $output, $rv);
  if ($rv == 1)
   guru_loadkernelmodule('coretemp');
 }
 $cputemp = array();
 for ($i = 0; $i <= 7; $i++)
 {
  $rawtemp = guru_sysctl('dev.cpu.'.(int)$i.'.temperature');
  if (@strlen($rawtemp) > 1)
   $cputemp[] = array(
    'CPUTEMP_CPUNR'	=> ($i + 1),
    'CPUTEMP_TEMP'	=> substr($rawtemp, 0, -1)
   );
  else
   break;
 }
 $cputemp_nosensor = (empty($cputemp)) ? 'normal' : 'hidden';

 // voltage sensors require mbmon to be installed
 $mbmon_path = '/usr/local/bin/mbmon';
 exec($mbmon_path.' -c1 -r', $mbmon, $rv);
 $class_need_mbmon = 'hidden';
 $class_mbmon_nosensor = 'hidden';
 if (!file_exists($mbmon_path))
  $class_need_mbmon = 'normal';
 elseif ($rv != 0)
  $class_mbmon_nosensor = 'normal';
 $mbmon_str = implode(chr(10), $mbmon);
 $mbmon = array();
 preg_match('/^TEMP0 \:(.*)$/m', $mbmon_str, $mbmon['temp0']);
 preg_match('/^TEMP1 \:(.*)$/m', $mbmon_str, $mbmon['temp1']);
 preg_match('/^TEMP2 \:(.*)$/m', $mbmon_str, $mbmon['temp2']);
 preg_match('/^FAN0  \:(.*)$/m', $mbmon_str, $mbmon['fan0']);
 preg_match('/^VC0   \:(.*)$/m', $mbmon_str, $mbmon['vcore0']);
 preg_match('/^VC1   \:(.*)$/m', $mbmon_str, $mbmon['vcore1']);
 preg_match('/^V33   \:(.*)$/m', $mbmon_str, $mbmon['v33']);
 preg_match('/^V50P  \:(.*)$/m', $mbmon_str, $mbmon['v50']);
 preg_match('/^V12P  \:(.*)$/m', $mbmon_str, $mbmon['v120']);
 foreach ($mbmon as $sensor_name => $pregdata)
 {
  $value = @trim($pregdata[1]);
  $sensor[$sensor_name] = $value;
  $sensorclass[$sensor_name] = 'hidden';
  if (strlen($value) > 1)
   if ((double)$value > 0)
    if ($sensor_name == 'fan0' OR ((double)$value < 99))
     if ($value{0} == '+')
     {
      $sensor[$sensor_name] = substr($value, 1);
      $sensorclass[$sensor_name] = 'normal';
     }
     else
      $sensorclass[$sensor_name] = 'normal';
 }

 // cpu frequency
 $cpu_freq = guru_sysctl('dev.cpu.0.freq');
 $class_cpu_freq = (is_numeric($cpu_freq)) ? 'normal' : 'hidden';
 $freqscaling = guru_sysctl('dev.cpu.0.freq_levels');
 $freqscaling_arr = explode(' ', $freqscaling);
 $freqrange = array();
 foreach ($freqscaling_arr as $rawtext)
  if (strpos($rawtext, '/') != false)
   $freqrange[] = (int)substr($rawtext, 0, strpos($rawtext, '/'));
 $cpu_freq_min = @min($freqrange);
 $cpu_freq_max = @max($freqrange);
 $cpu_freq_scaling = ((strlen($cpu_freq_min) > 0) AND 
  (strlen($cpu_freq_min) > 0)) ? 'normal' : 'hidden';

 // new tags produced by this content function
 $newtags = array(
  'SCRIPT_DATE'			=> $script_date,
  'SYSTEM_DIST'			=> $currentver['dist'],
  'SYSTEM_VERSION'		=> $currentver['sysver'],
  'SYSTEM_MD5'			=> $currentver['md5'],
  'BSD_VERSION'			=> $fbsdver,
  'ZFS_SPA'			=> $zfs_spa,
  'ZFS_ZPL'			=> $zfs_zpl,

  'PROCESSOR_STRING'		=> $cpu,
  'PROCESSOR_ARCH'		=> $arch,
  'CLASS_CPU_FREQ'		=> $class_cpu_freq,
  'CPU_FREQ'			=> $cpu_freq,
  'CPU_FREQ_SCALING'		=> $cpu_freq_scaling,
  'CPU_FREQ_MIN'		=> $cpu_freq_min,
  'CPU_FREQ_MAX'		=> $cpu_freq_max,
  'MEMORY_HUMAN'		=> $mem_human,
  'KMEM_HUMAN'			=> $kmem_human,
  'KMEM_MAX_HUMAN'		=> $kmem_max_human,

  'TABLE_STATUS_CPUTEMP'	=> $cputemp,
  'CPUTEMP_NOSENSOR'		=> $cputemp_nosensor,
  'CLASS_NEED_MBMON'		=> $class_need_mbmon,
  'CLASS_MBMON_NOSENSOR'	=> $class_mbmon_nosensor,
  'CLASS_TEMP0'			=> $sensorclass['temp0'],
  'CLASS_TEMP1'			=> $sensorclass['temp1'],
  'CLASS_TEMP2'			=> $sensorclass['temp2'],
  'CLASS_FAN0'			=> $sensorclass['fan0'],
  'CLASS_VCORE0'		=> $sensorclass['vcore0'],
  'CLASS_VCORE1'		=> $sensorclass['vcore1'],
  'CLASS_V33'			=> $sensorclass['v33'],
  'CLASS_V50'			=> $sensorclass['v50'],
  'CLASS_V120'			=> $sensorclass['v120'],
  'SENSOR_TEMP0'		=> $sensor['temp0'],
  'SENSOR_TEMP1'		=> $sensor['temp1'],
  'SENSOR_TEMP2'		=> $sensor['temp2'],
  'SENSOR_FAN0'			=> $sensor['fan0'],
  'SENSOR_VCORE0'		=> $sensor['vcore0'],
  'SENSOR_VCORE1'		=> $sensor['vcore1'],
  'SENSOR_V33'			=> $sensor['v33'],
  'SENSOR_V50'			=> $sensor['v50'],
  'SENSOR_V120'			=> $sensor['v120'],
  'SYSTEM_TIME'			=> $systime,
  'SYSTEM_UPTIME'		=> $uptime,
  'SYSTEM_LOADAVG'		=> $loadavg,
  'SYSTEM_CPUUSAGE'		=> $cpuusage,
  'SYSTEM_PHYSDISKS_STRING'	=> $diskcount
 );

 return $newtags;
}

?>
