<?php

function submit_system_export()
// export script and send via HTTP as downloadable package
{
 global $guru;
 // name and path of compressed file to create
 $filename = $guru['product_name'].'-'.$guru['product_version_string'].'.tgz';
 $filepath = $guru['tempdir'] . $filename;
 // construct tar command
 $command = '/usr/bin/tar cfz '.$filepath.' -C '.$guru['docroot'].' *';
 exec($command, $output, $rv);
 if ($rv != 0)
  error('Got return value '.(int)$rv.' while executing "'.$command.'".');
 if (!is_readable($filepath))
  error('Could not export scripts: tar script not readable or nonexistent.');
 // now send the file in the tempdir via HTTP
 header('Content-Type: application/x-tar');
 header('Content-Disposition: attachment; filename="'.$filename.'"');
 header('Content-Transfer-Encoding: binary');
 header('Content-Length: ' . (int)filesize($filepath));
 // flush buffer
 ob_clean();
 flush();
 // send file via HTTP
 $result = readfile($filepath);
 // delete file again
 exec('/bin/rm '.$filepath);
 // check result
 if ($result === false)
  error('Error reading file "'.$filepath.'"');
 // and die silently
 die();
}

?>
