<?php

function content_system_install_progress()
{
 global $guru;

 // variables
 $refresh_sec = 2;

 // installation output file
 $outputfile = $guru['tempdir'].'/zfsguru_install_output.txt';
 $fileexists = (file_exists($outputfile)) ? true : false;

 // installation output
 $outputfile_contents = '';
 if (file_exists($outputfile))
  $outputfile_contents = @htmlentities(file_get_contents($outputfile));

 // current job (current installation task)
 $oftag = '* ';
 $pos = strrpos($outputfile_contents, $oftag);
 if ($pos !== false)
 {
  $substr = substr($outputfile_contents, $pos + strlen($oftag));
  $pos2 = strpos($substr, chr(10));
  $currentjob = substr($substr, 0, $pos2);
 }
 else
  $currentjob = '';

 // visible classes
 $class_success = 'hidden';
 $class_failure = 'hidden';
 $class_installing = 'hidden';
 $class_output = (strlen($outputfile_contents) > 0) ? 'normal' : 'hidden';
 if (strpos($outputfile_contents, 'Reboot system now') !== false)
  $class_success = 'normal';
 elseif (strpos($outputfile_contents, 'ERROR:') !== false)
  $class_failure = 'normal';
 elseif ($fileexists)
  $class_installing = 'normal';
 else
  friendlyerror('no installation is active at this time', 'system.php?install');

 // automatic refresh during installation
 if ($class_installing == 'normal')
  page_refreshinterval($refresh_sec);

 // set default job
 if (($class_installing == 'normal') AND ($currentjob == ''))
  $currentjob = 'Initializing installation...';

 // export new tags
 return array(
  'PAGE_ACTIVETAB'	=> 'Install',
  'PAGE_TITLE'		=> 'Install',
  'CLASS_SUCCESS'	=> $class_success,
  'CLASS_FAILURE'	=> $class_failure,
  'CLASS_INSTALLING'	=> $class_installing,
  'CLASS_OUTPUT'	=> $class_output,
  'INSTALL_CURRENTJOB'	=> $currentjob,
  'INSTALL_OUTPUT'	=> $outputfile_contents
 );
}

?>
