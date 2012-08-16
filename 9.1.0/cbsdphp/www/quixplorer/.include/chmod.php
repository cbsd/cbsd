<?php
/*
	chmod.php
	
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
	Permission-Change Functions
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
function chmod_item($dir, $item) {		// change permissions
	if(($GLOBALS["permissions"]&01)!=01) show_error($GLOBALS["error_msg"]["accessfunc"]);
	if(!file_exists(get_abs_item($dir, $item))) show_error($item.": ".$GLOBALS["error_msg"]["fileexist"]);
	if(!get_show_item($dir, $item)) show_error($item.": ".$GLOBALS["error_msg"]["accessfile"]);
	
	// Execute
	if(isset($GLOBALS['__POST']["confirm"]) && $GLOBALS['__POST']["confirm"]=="true") {
		$bin='';
		for($i=0;$i<3;$i++) for($j=0;$j<3;$j++) {
			$tmp="r_".$i.$j;
			if(isset($GLOBALS['__POST'][$tmp]) &&$GLOBALS['__POST'][$tmp]=="1" ) $bin.='1';
			else $bin.='0';

			if($bin=='0') show_error($item.": ".$GLOBALS["error_msg"]["chmod_not_allowed"]); // Remove permissions from owner is not allowed!
			
		}
		
		if(!@chmod(get_abs_item($dir,$item),bindec($bin))) {
			show_error($item.": ".$GLOBALS["error_msg"]["permchange"]);
		}
		header("Location: ".make_link("link",$dir,NULL));
		return;
	}
	
	$mode = parse_file_perms(get_file_perms($dir,$item));
	if($mode===false) show_error($item.": ".$GLOBALS["error_msg"]["permread"]);
	$pos = "rwx";
	
	$s_item=get_rel_item($dir,$item);	if(strlen($s_item)>50) $s_item="...".substr($s_item,-47);
	show_header($GLOBALS["messages"]["actperms"].": /".$s_item);
	

	// Form
	echo "<CENTER><BR><TABLE width=\"175\"><FORM method=\"post\" action=\"";
	echo make_link("chmod",$dir,$item) . "\">\n";
	echo "<INPUT type=\"hidden\" name=\"confirm\" value=\"true\">\n";
	
	// print table with current perms & checkboxes to change	
	for($i=0;$i<3;++$i) {
		echo "<TR><TD>" . $GLOBALS["messages"]["miscchmod"][$i] . "</TD>";
		for($j=0;$j<3;++$j) {
			echo "<TD>" . $pos{$j} . "&nbsp;<INPUT type=\"checkbox\"";
			if($mode{(3*$i)+$j} != "-") echo " checked";
			echo " name=\"r_" . $i.$j . "\" value=\"1\"></TD>";
		}
		echo "</TR>\n";
	}
	
	// Submit / Cancel
	echo "</TABLE>\n<BR><TABLE>\n<TR><TD>\n<INPUT type=\"submit\" value=\"".$GLOBALS["messages"]["btnchange"];
	echo "\"></TD>\n<TD><input type=\"button\" value=\"".$GLOBALS["messages"]["btncancel"];
	echo "\" onClick=\"javascript:location='".make_link("list",$dir,NULL)."';\">\n</TD></TR></FORM></TABLE><BR></CENTER>\n";
}
//------------------------------------------------------------------------------
?>
