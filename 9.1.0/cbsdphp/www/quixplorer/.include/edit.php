<?php
/*
	edit.php
	
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
	File-Edit Functions
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
function savefile($file_name) {			// save edited file
	$code = stripslashes($GLOBALS['__POST']["code"]);
	$fp = @fopen($file_name, "w");
	if($fp===false) show_error(basename($file_name).": ".$GLOBALS["error_msg"]["savefile"]);
	fputs($fp, $code);
	@fclose($fp);
}
//------------------------------------------------------------------------------
function edit_file($dir, $item) {		// edit file
	if(($GLOBALS["permissions"]&01)!=01) show_error($GLOBALS["error_msg"]["accessfunc"]);
	if(!get_is_file($dir, $item)) show_error($item.": ".$GLOBALS["error_msg"]["fileexist"]);
	if(!get_show_item($dir, $item)) show_error($item.": ".$GLOBALS["error_msg"]["accessfile"]);
	
	$fname = get_abs_item($dir, $item);
	
	if(isset($GLOBALS['__POST']["dosave"]) && $GLOBALS['__POST']["dosave"]=="yes") {
		// Save / Save As
		$item=basename(stripslashes($GLOBALS['__POST']["fname"]));
		$fname2=get_abs_item($dir, $item);
		if(!isset($item) || $item=="") show_error($GLOBALS["error_msg"]["miscnoname"]);
		if($fname!=$fname2 && @file_exists($fname2)) show_error($item.": ".$GLOBALS["error_msg"]["itemdoesexist"]);
		savefile($fname2);
		$fname=$fname2;
	}
	
	// open file
	$fp = @fopen($fname, "r");
	if($fp===false) show_error($item.": ".$GLOBALS["error_msg"]["openfile"]);
	
	// header
	$s_item=get_rel_item($dir,$item);	if(strlen($s_item)>50) $s_item="...".substr($s_item,-47);
	show_header($GLOBALS["messages"]["actedit"].": /".$s_item);
	
	// Wordwrap (works only in IE)
?><script language="JavaScript1.2" type="text/javascript">
<!--
	function chwrap() {
		if(document.editfrm.wrap.checked) {
			document.editfrm.code.wrap="soft";
		} else {
			document.editfrm.code.wrap="off";
		}
	}
// -->
</script><?php

	// Form
	echo "<CENTER><BR><FORM name=\"editfrm\" method=\"post\" action=\"".make_link("edit",$dir,$item)."\">\n";
	echo "<input type=\"hidden\" name=\"dosave\" value=\"yes\">\n";
	echo "<b>Currently Editing: /".$dir."/".$item."</b>\n";
	echo "<TEXTAREA NAME=\"code\" rows=\"40\" cols=\"125\" wrap=\"off\">";
		
	// Show File In TextArea
	$buffer="";
	while(!feof ($fp)) {
		$buffer .= fgets($fp, 4096);
	}
	@fclose($fp);
	echo htmlspecialchars($buffer);
	
	echo "</TEXTAREA><BR>\n<TABLE><TR><TD>Wordwrap: (IE only)</TD><TD><INPUT type=\"checkbox\" name=\"wrap\" ";
	echo "onClick=\"javascript:chwrap();\" value=\"1\"></TD></TR></TABLE><BR>\n";
	echo "<TABLE><TR><TD><INPUT type=\"text\" name=\"fname\" value=\"".$item."\"></TD>";
	echo "<TD><input type=\"submit\" value=\"".$GLOBALS["messages"]["btnsave"];
	echo "\"></TD>\n<TD><input type=\"reset\" value=\"".$GLOBALS["messages"]["btnreset"]."\"></TD>\n<TD>";
	echo "<input type=\"button\" value=\"".$GLOBALS["messages"]["btnclose"]."\" onClick=\"javascript:location='";
	echo make_link("list",$dir,NULL)."';\"></TD></TR></FORM></TABLE><BR></CENTER>\n";
?><script language="JavaScript1.2" type="text/javascript">
<!--
	if(document.editfrm) document.editfrm.code.focus();
// -->
</script><?php
}
//------------------------------------------------------------------------------
?>
