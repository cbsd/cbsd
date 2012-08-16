<?php
/*
	del.php
	
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
	QuiXplorer Version 2.3.2
	File-Delete Functions
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
function del_items($dir) {		// delete files/dirs
	if(($GLOBALS["permissions"]&01)!=01) show_error($GLOBALS["error_msg"]["accessfunc"]);
	
	$cnt=count($GLOBALS['__POST']["selitems"]);
	$err=false;
	
	// delete files & check for errors
	for($i=0;$i<$cnt;++$i) {
		$items[$i] = stripslashes($GLOBALS['__POST']["selitems"][$i]);
		$abs = get_abs_item($dir,$items[$i]);
	
		if(!@file_exists(get_abs_item($dir, $items[$i]))) {
			$error[$i]=$GLOBALS["error_msg"]["itemexist"];
			$err=true;	continue;
		}
		if(!get_show_item($dir, $items[$i])) {
			$error[$i]=$GLOBALS["error_msg"]["accessitem"];
			$err=true;	continue;
		}
		
		// Delete
		$ok=remove(get_abs_item($dir,$items[$i]));
		
		if($ok===false) {
			$error[$i]=$GLOBALS["error_msg"]["delitem"];
			$err=true;	continue;
		}
		
		$error[$i]=NULL;
	}
	
	if($err) {			// there were errors
		$err_msg="";
		for($i=0;$i<$cnt;++$i) {
			if($error[$i]==NULL) continue;
			
			$err_msg .= $items[$i]." : ".$error[$i]."<BR>\n";
		}
		show_error($err_msg);
	}
	
	header("Location: ".make_link("list",$dir,NULL));
}
//------------------------------------------------------------------------------
?>
