<?php
/*
	lib_zip.php
	
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of Quixplorer (http://quixplorer.sourceforge.net).
	Author: The QuiX project.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met: 

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer. 
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies, 
	either expressed or implied, of the NAS4Free Project.
*/
/*------------------------------------------------------------------------------
Author: The QuiX project
	http://quixplorer.sourceforge.net
	
Comment:
	ZipFile class
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
class ZipFile {
//------------------------------------------------------------------------------
// Internal  vars
	var $datasec = array();					// Compressed data
	var $ctrl_dir = array();					// Central directory
	var $eof_ctrl_dir = "\x50\x4b\x05\x06\x00\x00\x00\x00";		// EOF directory record
	var $old_offset = 0;					// Last offset position
//------------------------------------------------------------------------------
// Internal function
	function unix2dos_time($unixtime=0) {
		//	Convert an Unix timestamp to a four byte DOS date and time format
		//	(date in high two bytes, time in low two bytes allowing magnitude comparison).
		$timearray = ($unixtime==0)?getdate():getdate($unixtime);
		if ($timearray['year'] < 1980) {
			$timearray['year'] = 1980;
			$timearray['mon'] = 1;
			$timearray['mday'] = 1;
			$timearray['hours'] = 0;
			$timearray['minutes'] = 0;
			$timearray['seconds'] = 0;
		}
		return (($timearray['year']-1980) << 25) | ($timearray['mon'] << 21) | ($timearray['mday'] << 16) |
			($timearray['hours'] << 11) | ($timearray['minutes'] << 5) | ($timearray['seconds'] >> 1);
	}
//------------------------------------------------------------------------------
// Data functions
	function add_data($data, $name, $time=0) {
		$name = str_replace('\\', '/', $name);
		$dtime = dechex($this->unix2dos_time($time));
		$hexdtime = '\x'.$dtime[6].$dtime[7].'\x'.$dtime[4].$dtime[5].'\x'.$dtime[2].$dtime[3].'\x'.$dtime[0].$dtime[1];
        		eval('$hexdtime = "' . $hexdtime . '";');
		
		$fr   = "\x50\x4b\x03\x04";
		$fr   .= "\x14\x00";		// ver needed to extract
		$fr   .= "\x00\x00";		// gen purpose bit flag
		$fr   .= "\x08\x00";		// compression method
		$fr   .= $hexdtime;		// last mod time and date

		// "local file header" segment
		$unc_len	= strlen($data);
		$crc	= crc32($data);
		$zdata	= gzcompress($data);
		$zdata	= substr(substr($zdata, 0, strlen($zdata) - 4), 2); // fix crc bug
		$c_len	= strlen($zdata);
		$fr	.= pack('V', $crc);		// crc32
		$fr	.= pack('V', $c_len);           // compressed filesize
		$fr	.= pack('V', $unc_len);	// uncompressed filesize
		$fr	.= pack('v', strlen($name));	// length of filename
		$fr	.= pack('v', 0);		// extra field length
		$fr	.= $name;

		// "file data" segment
		$fr .= $zdata;

		// "data descriptor" segment (optional but necessary if archive is not
		// served as file)
		$fr .= pack('V', $crc);		// crc32
		$fr .= pack('V', $c_len);		// compressed filesize
		$fr .= pack('V', $unc_len);		// uncompressed filesize

		// add this entry to array
		$this->datasec[] = $fr;
		$new_offset = strlen(implode('', $this->datasec));

		// now add to central directory record
		$cdrec = "\x50\x4b\x01\x02";
		$cdrec .= "\x00\x00";			// version made by
		$cdrec .= "\x14\x00";			// version needed to extract
		$cdrec .= "\x00\x00";			// gen purpose bit flag
		$cdrec .= "\x08\x00";			// compression method
		$cdrec .= $hexdtime;			// last mod time & date
		$cdrec .= pack('V', $crc);			// crc32
		$cdrec .= pack('V', $c_len);			// compressed filesize
		$cdrec .= pack('V', $unc_len);		// uncompressed filesize
		$cdrec .= pack('v', strlen($name));		// length of filename
		$cdrec .= pack('v', 0 );			// extra field length
		$cdrec .= pack('v', 0 );			// file comment length
		$cdrec .= pack('v', 0 );			// disk number start
		$cdrec .= pack('v', 0 );			// internal file attributes
		$cdrec .= pack('V', 32 );			// external file attributes - 'archive' bit set
		
		$cdrec .= pack('V', $this->old_offset );	// relative offset of local header
		$this->old_offset = $new_offset;
		
		$cdrec .= $name;

		// optional extra field, file comment goes here
		// save to central directory
		$this->ctrl_dir[] = $cdrec;
	}
	
	function contents() {
		$data = implode('', $this->datasec);
		$ctrldir = implode('', $this->ctrl_dir);
		return $data.$ctrldir.$this->eof_ctrl_dir.
			pack('v', sizeof($this->ctrl_dir)).	// total # of entries "on this disk"
			pack('v', sizeof($this->ctrl_dir)).	// total # of entries overall
			pack('V', strlen($ctrldir)).		// size of central dir
			pack('V', strlen($data)).		// offset to start of central dir
			"\x00\x00";			// .zip file comment length
	}
//------------------------------------------------------------------------------
// File functions
	function add($dir, $name) {
		$item=$dir."/".$name;
		
		if(@is_file($item)) {
			if(($fp=fopen($item,"rb"))===false) return false;
			$data=fread($fp,filesize($item));
			fclose($fp);
			$this->add_data($data,$name,filemtime($item));
			return true;
		} elseif(@is_dir($item)) {
			if(($handle=opendir($item))===false) return false;
			while(($file=readdir($handle))!==false) {
				if(($file==".." || $file==".")) continue;
				if(!$this->add($dir,$name."/".$file)) return false;
			}
			closedir($handle);
			return true;
		}
		
		return false;
	}
	
	function save($name) {
		if(($fp=fopen($name,"wb"))===false) return false;
		fwrite($fp, $this->contents());
		fclose($fp);
		return true;
	}
}
//------------------------------------------------------------------------------
?>