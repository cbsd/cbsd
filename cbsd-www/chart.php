<?php

/*
** ZFSguru - chart.php
** creates benchmark charts (.png images) from benchmark data
*/

// functions

function zfsguru_createbenchmarkchart($chart_name, $data)
// returns a GD-lib chart based on $data array
{
 // check for GD-lib
 if (!function_exists('imagecreate'))
  return false;

 // set memory limit
 ini_set("memory_limit","512M");

 // first analyze $benchmark to determine maxscore
 $maxscore = 0;
 $maxdisks = 1;
 foreach ($data['benchmark'] as $colortag => $scoreset)
  foreach ($scoreset as $diskcount => $score)
  {
   $maxscore = ($score > $maxscore) ? $score : $maxscore;
   $maxdisks = (($diskcount > $maxdisks) AND 
    ($score > 0)) ? $diskcount : $maxdisks;
  }

 // calculate all numbers
 if ($maxscore > 50000)
  $score_y_mib = 10000;
 elseif ($maxscore > 25000)
  $score_y_mib = 5000;
 elseif ($maxscore > 10000)
  $score_y_mib = 2000;
 elseif ($maxscore > 5000)
  $score_y_mib = 1000;
 elseif ($maxscore > 2000)
  $score_y_mib = 500;
 elseif ($maxscore > 1000)
  $score_y_mib = 200;
 elseif ($maxscore > 100)
  $score_y_mib = 100;
 elseif ($maxscore > 20)
  $score_y_mib = 20;
 elseif ($maxscore > 10)
  $score_y_mib = 5;
 else
  $score_y_mib = 2;
 $testcount = (int)count($data['benchmark']);
 $units_x = $maxdisks;
 $units_y = (int)ceil($maxscore / $score_y_mib);
 $units_y_mib = $units_y * $score_y_mib;
 $resolution_x = 60;
 $resolution_y = 60;
 $pitch_box = 12;
 $margin = array('left' => 10, 'top' => 10, 'right' => 10, 'bottom' => 10);
 $frame = array();
 $frame['start'] = array('x' => 20, 'y' => '20');
 $frame['end'] = array(
  'x' => ($frame['start']['x'] + ($units_x * $resolution_x)),
  'y' => ($frame['start']['y'] + ($units_y * $resolution_y))
 );
 $font = 'files/liberationsans.ttf';
 $width = $frame['end']['x'] + $margin['right'];
 $height = $frame['end']['y'] + $margin['bottom'] + 
  22 + ($testcount * $pitch_box) + 70;
 if ($width > 10000)
  return false;
 if ($height > 10000)
  return false;
 if ($width < 211)
  $width = 211;

 // create image
 $image = imagecreatetruecolor($width, $height);
 // fail if image was not created (memory problems?)
 if (!is_resource($image))
  return false;

 // set colors
 $colors = array(
  'bg' => imagecolorallocate($image, 250, 250, 250),
  'title' => imagecolorallocate($image, 100, 100, 100),
  'txt' => imagecolorallocate($image, 100, 100, 100),
  'frame' => imagecolorallocate($image, 0, 0, 0),
  'databg' => imagecolorallocate($image, 255, 255, 255),
  'running' => imagecolorallocate($image, 255, 0, 0),
  'wm1' => imagecolorallocate($image, 255, 255, 255),
  'wm2' => imagecolorallocate($image, 220, 220, 220),
  'grid' => imagecolorallocate($image, 220, 220, 220),
  'RAID0' => imagecolorallocate($image, 233, 14, 91),
  'RAID1' => imagecolorallocate($image, 114, 114, 120),
  'RAID1+0' => imagecolorallocate($image, 233, 214, 91),
  'RAIDZ' => imagecolorallocate($image, 14, 14, 120),
  'RAIDZ2' => imagecolorallocate($image, 133, 55, 171),
  'RAIDZ+0' => imagecolorallocate($image, 100, 255, 171),
  'RAIDZ2+0' => imagecolorallocate($image, 20, 255, 171)
 );

 // begin working on image
 @imageantialias($image, true);
 imagefill($image, 0, 0, $colors['bg']);

 // draw border
 imagerectangle($image, 0, 0, $width-1 , $height-1 , $colors['txt']);

 // draw watermark
 $watermark = $data['sysinfo']['product_name'];
 imagettftext($image, 7, 0, $width - 41, 11, $colors['wm1'],
  $font, $watermark);
 imagettftext($image, 7, 0, $width - 40, 10, $colors['wm2'],
  $font, $watermark);

 // write chart name
 imagettftext($image, 11, 0, 15, 15, $colors['title'], $font, $chart_name);

// draw grid
 for ($i = $frame['start']['x']; $i <= $frame['end']['x']; $i += $resolution_x)
// if (($i > $frame['start']['x']) AND ($i < $frame['end']['x']))
  imageline($image, $i, $frame['start']['y']+1, $i, 
   $frame['end']['y']-1, $colors['grid']);
 for ($i = $frame['start']['y']; $i <= $frame['end']['y']; $i += $resolution_y)
// if (($i > $start['y']) AND ($i < $end['y']))
  imageline($image, $frame['start']['x']+1, $i, $frame['end']['x']-1, 
   $i, $colors['grid']);
 imageline($image, $frame['start']['x'], $frame['start']['y'], 
  $frame['start']['x'], $frame['end']['y'], $colors['frame']);
 imageline($image, $frame['start']['x'], $frame['end']['y'], 
  $frame['end']['x'], $frame['end']['y'], $colors['frame']);

 // draw horizontal units
 for ($i = 1; $i <= $units_x; $i++)
  imagettftext($image, 9, 0, $frame['start']['x'] - 3 + ($i * $resolution_x), 
   $frame['end']['y'] + 15, $colors['txt'], $font, $i);

 // draw vertical units
 $start = array('x' => 2, 'y' => 305);
 $step = array('x' => 0, 'y' => (-1 * $resolution_y));
 for ($i = 0; $i <= $units_y; $i++)
  imagettftext($image, 7, 0, 2,
   $frame['end']['y'] + 4 + ($i * $step['y']), $colors['txt'], $font, 
    $i * $score_y_mib);

 // add "benchmark running" watermark if applicable
 if (@$data['benchmarkrunning'])
  imagettftext($image, 7, 0, $frame['start']['x']+5, $frame['end']['y'] - 6 ,
   $colors['running'], $font, 'Benchmark still running');

 // process benchmark data
 $startoffset = $frame['start']['x'] + $resolution_x;
 $pitch = $resolution_x;
 foreach ($data['benchmark'] as $colortag => $scoreset)
  foreach ($scoreset as $diskcount => $score)
   if ($score > 0)
   {
    $frame_y = $frame['end']['y'] - $frame['start']['y'];
    $scorepx = (int)($score * ($frame_y / $units_y_mib));
    // draw cross on scorepx
    $x1 = $startoffset + ( $pitch * ($diskcount-1) );
    $y1 = $frame['end']['y'] - $scorepx;
    $csize = 4;
    imageline($image, $x1-$csize, $y1, $x1+$csize, $y1, $colors[$colortag]);
    imageline($image, $x1, $y1-$csize, $x1, $y1+$csize, $colors[$colortag]);
    // draw chart line
    $nextscorepx = 0;
    for ($i = $diskcount; $i <= $maxdisks; $i++)
    {
     // skip first entry (we want next score)
     if ($i == $diskcount)
      continue;
     $nextscorepx = @(int)($scoreset[$i] * ($frame_y / $units_y_mib));
     if ($nextscorepx > 0)
      break;
    }
    if (@$nextscorepx > 0)
    {
     $x2 = $startoffset + ($pitch * ($i-1) ) ;
     $y2 = $frame['end']['y'] - $nextscorepx;
     imageboldline($image, $x1, $y1, $x2, $y2, $colors[$colortag]);
    }
   }

 // draw benchmark detail box
 $startoffset = $frame['start']['x'] + $resolution_x;
 $benchid = 1;
 foreach ($data['benchmark'] as $colortag => $scoreset)
 {
  $x = $frame['start']['x'];
  $y = $frame['end']['y'] + $frame['start']['y'] + ( $pitch_box * $benchid );
  imagettftext($image, 7, 0, $x, $y, $colors[$colortag], $font, $colortag);

  foreach ($scoreset as $diskcount => $score)
   if ($score > 0)
   {
    $x = $startoffset + ( $resolution_x * 0.5 * ($diskcount-1) );
    $y = $frame['end']['y'] + $frame['start']['y'] + ( $pitch_box * $benchid );
    // draw box first; twice so we get a nice filled box with border
    imagefilledrectangle($image, $x - 4, $y - 10, $x + 26, $y + 2,
     $colors['databg']);
    imagerectangle($image, $x - 4, $y - 10, $x + 26, $y + 2, $colors['grid']);
    // now write the text on top
    imagettftext($image, 7, 0, $x, $y, $colors[$colortag], $font, $score);
   }
  $benchid++;
 }

 // draw system information box
 $x1 = $margin['left'];
 $y1 = $y + (1 * $pitch_box);
 $x2 = $width - $margin['right'];
 $y2 = $height - $margin['bottom'];
 imagefilledrectangle($image, $x1, $y1, $x2, $y2, $colors['databg']);
 imagerectangle($image, $x1, $y1, $x2, $y2, $colors['grid']);
 // gather data
 $cpuname = $data['sysinfo']['cpu_name'];
 $cpucount = $data['sysinfo']['cpu_count'];
 $cpufreq = $data['sysinfo']['cpu_freq_ghz'];
 $kmem = $data['sysinfo']['kmem_gib'];
 $physmem = $data['sysinfo']['physmem_gib'];
 $productversion = $data['sysinfo']['product_name'].' '
  .$data['sysinfo']['product_version'];
 $distribution = $data['sysinfo']['distribution'];
 $sysver = $data['sysinfo']['system_version'];
 @$txt = array(
  $productversion.' running '.$distribution.' '.$sysver,
  'CPU: '.$cpuname,
  'CPU freq: '.$cpucount.'x '.$cpufreq.' GHz; KMEM: '.$kmem.'/'.$physmem.' GiB',
  'Test settings: '.@$data['testsettings_str'],
  'Tuning: '.@$data['tuning_str']);
 // process data
 for ($i = 0; $i <= count($txt)-1; $i++)
  imagettftext($image, 7, 0, $x1+2, $y1+11+(11*$i), 
   $colors['txt'], $font, $txt[$i]);

 // return image
 return $image;
}

function imageboldline($image, $x1, $y1, $x2, $y2, $color, $thickness=2)
{
 $x1 -= ( $buf = ceil(($thickness-1) / 2) );
 $x2 -= $buf;
 for ($i=0; $i < $thickness; ++$i)
  imageline($image, $x1+$i, $y1, $x2+$i, $y2, $color);
}

?>
