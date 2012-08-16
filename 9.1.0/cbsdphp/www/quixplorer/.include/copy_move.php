<?php
/*
	copy_move.php
	
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
	File/Directory Copy & Move Functions
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
function dir_list($dir) {			// make list of directories
	// this list is used to copy/move items to a specific location
	
	$handle = @opendir(get_abs_dir($dir));
	if($handle===false) return;		// unable to open dir
	
	while(($new_item=readdir($handle))!==false) {
		//if(!@file_exists(get_abs_item($dir, $new_item))) continue;
		
		if(!get_show_item($dir, $new_item)) continue;
		if(!get_is_dir($dir,$new_item)) continue;
		$dir_list[$new_item] = $new_item;
	}
	
	// sort
	if(is_array($dir_list)) ksort($dir_list);
	return $dir_list;
}
//------------------------------------------------------------------------------
function dir_print($dir_list, $new_dir) {	// print list of directories
	// this list is used to copy/move items to a specific location
	
	// Link to Parent Directory
	$dir_up = dirname($new_dir);
	if($dir_up==".") $dir_up = "";
	
	echo "<CENTER><TR><TD><A HREF=\"javascript:NewDir('".$dir_up;
	echo "');\"><IMG border=\"0\" width=\"16\" height=\"16\"";
	echo " align=\"ABSMIDDLE\" src=\"_img/_up.gif\" ALT=\"\">&nbsp;..</A></TD></TR></CENTER>\n";
	
	// Print List Of Target Directories
	if(!is_array($dir_list)) return;
	while(list($new_item,) = each($dir_list)) {
		$s_item=$new_item;	if(strlen($s_item)>40) $s_item=substr($s_item,0,37)."...";
		echo "<CENTER><TR><TD><A HREF=\"javascript:NewDir('".get_rel_item($new_dir,$new_item).
			"');\"><IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ".
			"src=\"_img/dir.gif\" ALT=\"\">&nbsp;".$s_item."</A></TD></TR></CENTER>\n";
	}
}
//------------------------------------------------------------------------------
function copy_move_items($dir) {		// copy/move file/dir
	if(($GLOBALS["permissions"]&01)!=01) show_error($GLOBALS["error_msg"]["accessfunc"]);
	
	// Vars
	$first = $GLOBALS['__POST']["first"];
	if($first=="y") $new_dir=$dir;
	else $new_dir = stripslashes($GLOBALS['__POST']["new_dir"]);
	if($new_dir==".") $new_dir="";
	$cnt=count($GLOBALS['__POST']["selitems"]);

	// Copy or Move?
	if($GLOBALS["action"]!="move") {
		$_img="_img/__copy.gif";
	} else {
		$_img="_img/__cut.gif";
	}
	
	// Get New Location & Names
	if(!isset($GLOBALS['__POST']["confirm"]) || $GLOBALS['__POST']["confirm"]!="true") {
		show_header(($GLOBALS["action"]!="move"?
			$GLOBALS["messages"]["actcopyitems"]:
			$GLOBALS["messages"]["actmoveitems"]
		));
		
		// JavaScript for Form:
		// Select new target directory / execute action
?><script language="JavaScript1.2" type="text/javascript">
<!--
	function NewDir(newdir) {
		document.selform.new_dir.value = newdir;
		document.selform.submit();
	}
	
	function Execute() {
		document.selform.confirm.value = "true";
	}
//-->
</script><?php
		
		// "Copy / Move from .. to .."
		$s_dir=$dir;		if(strlen($s_dir)>40) $s_dir="...".substr($s_dir,-37);
		$s_ndir=$new_dir;	if(strlen($s_ndir)>40) $s_ndir="...".substr($s_ndir,-37);
		echo "<CENTER><BR><IMG SRC=\"".$_img."\" align=\"ABSMIDDLE\" ALT=\"\">&nbsp;";
		echo sprintf(($GLOBALS["action"]!="move"?$GLOBALS["messages"]["actcopyfrom"]:
			$GLOBALS["messages"]["actmovefrom"]),$s_dir, $s_ndir);
		echo "<IMG SRC=\"_img/__paste.gif\" align=\"ABSMIDDLE\" ALT=\"\"></CENTER>\n";
		
		// Form for Target Directory & New Names
		echo "<CENTER><BR><BR><FORM name=\"selform\" method=\"post\" action=\"";
		echo make_link("post",$dir,NULL)."\"><TABLE>\n";
		echo "<INPUT type=\"hidden\" name=\"do_action\" value=\"".$GLOBALS["action"]."\">\n";
		echo "<INPUT type=\"hidden\" name=\"confirm\" value=\"false\">\n";
		echo "<INPUT type=\"hidden\" name=\"first\" value=\"n\">\n";
		echo "<INPUT type=\"hidden\" name=\"new_dir\" value=\"".$new_dir."\"></CENTER>\n";
		
		// List Directories to select Target
		dir_print(dir_list($new_dir),$new_dir);
		echo "<CENTER></TABLE><BR><TABLE>\n";
		
		// Print Text Inputs to change Names
		for($i=0;$i<$cnt;++$i) {
			$selitem=stripslashes($GLOBALS['__POST']["selitems"][$i]);
			if(isset($GLOBALS['__POST']["newitems"][$i])) {
				$newitem=stripslashes($GLOBALS['__POST']["newitems"][$i]);
				if($first=="y") $newitem=$selitem;
			} else $newitem=$selitem;
			$s_item=$selitem;	if(strlen($s_item)>50) $s_item=substr($s_item,0,47)."...";
			echo "<TR><TD><IMG SRC=\"_img/_info.gif\" align=\"ABSMIDDLE\" ALT=\"\">";
			// Old Name
			echo "<INPUT type=\"hidden\" name=\"selitems[]\" value=\"";
			echo $selitem."\">&nbsp;".$s_item."&nbsp;";
			// New Name
			echo "</TD><TD><INPUT type=\"text\" size=\"25\" name=\"newitems[]\" value=\"";
			echo $newitem."\"></TD></TR>\n";
		}
		
		// Submit & Cancel
		echo "</TABLE><BR><TABLE><TR>\n<TD>";
		echo "<INPUT type=\"submit\" value=\"";
		echo ($GLOBALS["action"]!="move"?$GLOBALS["messages"]["btncopy"]:$GLOBALS["messages"]["btnmove"]);
		echo "\" onclick=\"javascript:Execute();\"></TD>\n<TD>";
		echo "<input type=\"button\" value=\"".$GLOBALS["messages"]["btncancel"];
		echo "\" onClick=\"javascript:location='".make_link("list",$dir,NULL);
		echo "';\"></TD>\n</TR></FORM></TABLE><BR></CENTER>\n";
		return;
	}
	
	
	// DO COPY/MOVE
	
	// ALL OK?
	if(!@file_exists(get_abs_dir($new_dir))) show_error($new_dir.": ".$GLOBALS["error_msg"]["targetexist"]);
	if(!get_show_item($new_dir,"")) show_error($new_dir.": ".$GLOBALS["error_msg"]["accesstarget"]);
	if(!down_home(get_abs_dir($new_dir))) show_error($new_dir.": ".$GLOBALS["error_msg"]["targetabovehome"]);
	
	
	// copy / move files
	$err=false;
	for($i=0;$i<$cnt;++$i) {
		$tmp = stripslashes($GLOBALS['__POST']["selitems"][$i]);
		$new = basename(stripslashes($GLOBALS['__POST']["newitems"][$i]));
		$abs_item = get_abs_item($dir,$tmp);
		$abs_new_item = get_abs_item($new_dir,$new);
		$items[$i] = $tmp;
	
		// Check
		if($new=="") {
			$error[$i]= $GLOBALS["error_msg"]["miscnoname"];
			$err=true;	continue;
		}
		if(!@file_exists($abs_item)) {
			$error[$i]= $GLOBALS["error_msg"]["itemexist"];
			$err=true;	continue;
		}
		if(!get_show_item($dir, $tmp)) {
			$error[$i]= $GLOBALS["error_msg"]["accessitem"];
			$err=true;	continue;
		}
		if(@file_exists($abs_new_item)) {
			$error[$i]= $GLOBALS["error_msg"]["targetdoesexist"];
			$err=true;	continue;
		}
	
		// Copy / Move
		if($GLOBALS["action"]=="copy") {
			if(@is_link($abs_item) || @is_file($abs_item)) {
				// check file-exists to avoid error with 0-size files (PHP 4.3.0)
				$ok=@copy($abs_item,$abs_new_item);	//||@file_exists($abs_new_item);
			} elseif(@is_dir($abs_item)) {
				$ok=copy_dir($abs_item,$abs_new_item);
			}
		} else {
			$ok=@rename($abs_item,$abs_new_item);
		}
		
		if($ok===false) {
			$error[$i]=($GLOBALS["action"]=="copy"?
				$GLOBALS["error_msg"]["copyitem"]:
				$GLOBALS["error_msg"]["moveitem"]
			);
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
