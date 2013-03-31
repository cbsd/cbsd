<?php

function content_pools_benchmark()
{
 global $tags;

 // required library
 activate_library('zfs');

 // fetch data
 $pools = zfs_pool_list();

 // poollist
 $poollist = array();
 foreach ($pools as $poolname => $data)
  if (($data['status'] == 'ONLINE') OR ($data['status'] == 'DEGRADED'))
   $poollist[] = array(
    'POOLNAME'	=> $poolname
   );

 // hide benchmark output if no form submitted
 $class_bench = (@isset($tags['POOLS_BENCHMARKOUTPUT']))
  ? 'normal' : 'hidden';

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'Benchmark',
  'PAGE_TITLE'		=> 'Benchmark',
  'TABLE_POOLLIST'	=> $poollist,
  'CLASS_BENCHMARK'	=> $class_bench,
 );
 return $newtags;
}

function submit_pools_benchmark()
{
 global $guru;

 // required libraries
 activate_library('super');
 activate_library('zfs');

 // sanitize input
 sanitize(@$_POST['poolname'], null, $poolname);

 // call function
 $poolfs = zfs_filesystem_list($poolname);
 $pool = zfs_pool_list($poolname);

 // variables
 $url = 'pools.php?benchmark';
 $size = @$_POST['size'];
 $type = @$_POST['type'];
 $source = @$_POST['source'];
 $mountpoint = @$poolfs[$poolname]['mountpoint'];
 $testfilename = 'zfsguru_benchmark.000';
 $capacity = $pool['size'];
 $capacity_pct = $pool['cap'];
 $testsize_gib = @((int)$size / 1024);

 // sanity
 if (strlen($poolname) < 1)
  error('sanity failure on pool name');
 if (!file_exists($mountpoint))
  error('sanity failure on mountpoint existence check');

 // dd input/output file
 $source = '/dev/'.$source;
 $testfile = $mountpoint.'/'.$testfilename;
 if ((@strlen($mountpoint) < 2) OR ($mountpoint{0} != '/'))
  error('Invalid mountpoint "'.$mountpoint.'"');

 // check if test file exists
 if (file_exists($testfile))
  error('Test file '.$testfile.' already exists!');

 // dd write command execution
 $command = '/bin/dd if='.$source.' of='.$testfile.' bs=1m '
  .'count='.(int)$size.' 2>&1';
 $result = super_execute($command);
 $rv1 = @$result['rv'];
 $writescore = @$result['output_arr'][2];

 // dd read command execution
 $command = '/bin/dd if='.$testfile.' of=/dev/null bs=1m 2>&1';
 $result = super_execute($command);
 $rv2 = @$result['rv'];
 $readscore = @$result['output_arr'][2];

 // remove test file
 super_execute('/bin/rm '.$testfile);

 // process scores
 preg_match('/\(([0-9]+) bytes\/sec\)$/', $readscore, $readspeed);
 preg_match('/\(([0-9]+) bytes\/sec\)$/', $writescore, $writespeed);
 $readspeed = @$readspeed[1];
 $writespeed = @$writespeed[1];

 // output string
 $outputstr = 
   '<b>ZFSguru</b> '.$guru['product_version_string'].' pool benchmark'.chr(10)
  .'Pool            : '.$poolname
  .' ('.$capacity.', <b>'.$capacity_pct.'</b> full)'.chr(10)
  .'Test size       : '.$testsize_gib.' GiB'.chr(10)
  .'Data source     : '.$source.chr(10)
  .'Read throughput : <b>'.sizehuman($readspeed, 1).'/s</b> = <b>'
   .sizebinary($readspeed, 1).'/s</b>'.chr(10)
  .'Write throughput: <b>'.sizehuman($writespeed, 1).'/s</b> = <b>'
   .sizebinary($writespeed, 1).'/s</b>';

 // return output
 return array(
  'POOLS_BENCHMARKOUTPUT' => $outputstr
 );
}

?>
