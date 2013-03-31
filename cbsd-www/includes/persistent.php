<?php

function persistent_read($section = false)
// read persistent storage file and return array
{
 // read serialized data from file
 $filename = 'config/persistent.dat';
 $contents = @file_get_contents($filename);
 $arr = @unserialize($contents);
 if (!is_array($arr))
  return false;
 elseif ($section == false)
  return $arr;
 elseif (@!isset($arr[$section]))
  return false;
 else
  return $arr[$section];
}

function persistent_write($arr)
// writes array with persistent data to file; removes file on empty array
{
 // write serialized array to file
 $filename = 'config/persistent.dat';
 if (is_array($arr) AND empty($arr))
  return @unlink($filename);
 else
 {
  $ser = serialize($arr);
  return file_put_contents($filename, $ser);
 }
}

function persistent_store($sectionname, $data)
// stores section (array with $sectionname => $data) to persistent storage
{
 // read data
 $arr = persistent_read();
 // add new section
 $arr[$sectionname] = $data;
 // write data
 return persistent_write($arr);
}

function persistent_remove($sectionname = false)
// removes section from persistent storage
{
 // read data
 $arr = persistent_read();
 if (!is_array($arr))
  $arr = array();
 // remove section
 if (@isset($arr[$sectionname]))
  unset($arr[$sectionname]);
 // write data
 return persistent_write($arr);
}

?>
