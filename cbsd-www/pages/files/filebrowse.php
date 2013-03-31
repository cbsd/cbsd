<?php

function content_files_filebrowse()
{
 // working directory
 if (strlen($_GET['browse']) > 0)
 {
  $wd = $_GET['browse'];
  // redirect if path differs from realpath; for .. and symlinks
  if (realpath($wd) != $wd)
   redirect_url('files.php?browse='
    .str_replace('%2F', '/', urlencode(realpath($wd))));
 }
 else
  $wd = '/';

 // fetch directory contents
 $command = '/bin/ls -la '.$wd;
 exec($command, $ls);
 $lsarr = array();
 if (is_array($ls))
  foreach ($ls as $line)
  {
   $split = preg_split('/[\s]+/m', $line, 9);
   $file = @$split[8];
   $arrstr = '';
   if (@strlen($file) > 0)
   {
    $firstchunk = substr($line,0,strrpos($line, $file));
    $lastchunk = substr($line,strrpos($line, $file));
    if ($lastchunk == $file)
    {
     // check for symbolic links
     $pos = strpos($file, ' -> ');
     if ($pos !== false)
      $file_name = substr($file, 0, strpos($file, ' -> '));
     else
      $file_name = $file;

     $lastchunk = '<a href="files.php?browse='.htmlentities(
      $wd.'/'.$file_name).'">'
      .htmlentities($file).'</a>';
    }
    $lsarr[] = htmlentities($firstchunk) . $lastchunk;
   }
   else
    $lsarr[] = htmlentities($line);
//   for ($i = 0; $i <= 7; $i++)
//    $arrstr .= @htmlentities($split[$i]);
//   $lsarr[] = str_replace($file, $hyperlink, $line);
  }

 // browse box
 $browsebox = implode(chr(10), $lsarr);

 // new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'File browser',
  'PAGE_TITLE'		=> 'File browser',
  'FILES_WD'		=> $wd,
  'FILES_BROWSEBOX'	=> $browsebox
 );
 return $newtags;
}

?>
