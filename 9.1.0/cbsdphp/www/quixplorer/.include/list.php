<?php
/*
	list.php
	
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
	Directory-Listing Functions
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
// HELPER FUNCTIONS (USED BY MAIN FUNCTION 'list_dir', SEE BOTTOM)
function make_list($_list1, $_list2) {		// make list of files
	$list = array();

	if($GLOBALS["srt"]=="yes") {
		$list1 = $_list1;
		$list2 = $_list2;
	} else {
		$list1 = $_list2;
		$list2 = $_list1;
	}
	
	if(is_array($list1)) {
		while (list($key, $val) = each($list1)) {
			$list[$key] = $val;
		}
	}
	
	if(is_array($list2)) {
		while (list($key, $val) = each($list2)) {
			$list[$key] = $val;
		}
	}
	
	return $list;
}
//------------------------------------------------------------------------------
function make_tables($dir, &$dir_list, &$file_list, &$tot_file_size, &$num_items)
{						// make table of files in dir
	// make tables & place results in reference-variables passed to function
	// also 'return' total filesize & total number of items
	
	$tot_file_size = $num_items = 0;
	
	// Open directory
	$handle = @opendir(get_abs_dir($dir));
	if($handle===false) show_error($dir.": ".$GLOBALS["error_msg"]["opendir"]);
	
	// Read directory
	while(($new_item = readdir($handle))!==false) {
		$abs_new_item = get_abs_item($dir, $new_item);
		
		if(!@file_exists($abs_new_item)) show_error($dir.": ".$GLOBALS["error_msg"]["readdir"]);
		if(!get_show_item($dir, $new_item)) continue;
		
		$new_file_size = filesize($abs_new_item);
		$tot_file_size += $new_file_size;
		$num_items++;
		
		if(get_is_dir($dir, $new_item)) {
			if($GLOBALS["order"]=="mod") {
				$dir_list[$new_item] =
					@filemtime($abs_new_item);
			} else {	// order == "size", "type" or "name"
				$dir_list[$new_item] = $new_item;
			}
		} else {
			if($GLOBALS["order"]=="size") {
				$file_list[$new_item] = $new_file_size;
			} elseif($GLOBALS["order"]=="mod") {
				$file_list[$new_item] =
					@filemtime($abs_new_item);
			} elseif($GLOBALS["order"]=="type") {
				$file_list[$new_item] =
					get_mime_type($dir, $new_item, "type");
			} else {	// order == "name"
				$file_list[$new_item] = $new_item;
			}
		}
	}
	closedir($handle);
	
	
	// sort
	if(is_array($dir_list)) {
		if($GLOBALS["order"]=="mod") {
			if($GLOBALS["srt"]=="yes") arsort($dir_list);
			else asort($dir_list);
		} else {	// order == "size", "type" or "name"
			if($GLOBALS["srt"]=="yes") ksort($dir_list);
			else krsort($dir_list);
		}
	}
	
	// sort
	if(is_array($file_list)) {
		if($GLOBALS["order"]=="mod") {
			if($GLOBALS["srt"]=="yes") arsort($file_list);
			else asort($file_list);
		} elseif($GLOBALS["order"]=="size" || $GLOBALS["order"]=="type") {
			if($GLOBALS["srt"]=="yes") asort($file_list);
			else arsort($file_list);
		} else {	// order == "name"
			if($GLOBALS["srt"]=="yes") ksort($file_list);
			else krsort($file_list);
		}
	}
}
//------------------------------------------------------------------------------
function print_table($dir, $list, $allow) {	// print table of files
	if(!is_array($list)) return;
	
	while(list($item,) = each($list)){
		// link to dir / file
		$abs_item=get_abs_item($dir,$item);
		$target="";
		//$extra="";
		//if(is_link($abs_item)) $extra=" -> ".@readlink($abs_item);
		if(is_dir($abs_item)) {
			$link = make_link("list",get_rel_item($dir, $item),NULL);
		} else { //if(get_is_editable($dir,$item) || get_is_image($dir,$item)) {
//?? CK Hier wird kuenftig immer mit dem download-Link gearbeitet, damit
//?? CK die Leute links klicken koennen
//?? CK			$link = $GLOBALS["home_url"]."/".get_rel_item($dir, $item);
			$link = make_link("download", $dir, $item);
			$target = "_blank";
		} //else $link = "";
		
		echo "<TR class=\"rowdata\"><TD><INPUT TYPE=\"checkbox\" name=\"selitems[]\" value=\"";
		echo htmlspecialchars($item)."\" onclick=\"javascript:Toggle(this);\"></TD>\n";
	// Icon + Link
		echo "<TD nowrap>";
		/*if($link!="") */ echo"<A HREF=\"" . $link . "\">";
		//else echo "<A>";
		echo "<IMG border=\"0\" width=\"16\" height=\"16\" ";
		echo "align=\"ABSMIDDLE\" src=\"_img/".get_mime_type($dir, $item, "img")."\" ALT=\"\">&nbsp;";
		$s_item=$item;	if(strlen($s_item)>50) $s_item=substr($s_item,0,47)."...";
		echo htmlspecialchars($s_item)."</A></TD>\n";	// ...$extra...
	// Size
		echo "<TD>".parse_file_size(get_file_size($dir,$item))."</TD>\n";
	// Type
		echo "<TD>".get_mime_type($dir, $item, "type")."</TD>\n";
	// Modified
		echo "<TD>".parse_file_date(get_file_date($dir,$item))."</TD>\n";
	// Permissions
		echo "<TD>";
		if($allow) {
			echo "<A HREF=\"".make_link("chmod",$dir,$item)."\" TITLE=\"";
			echo $GLOBALS["messages"]["permlink"]."\">";
		}
		echo parse_file_type($dir,$item).parse_file_perms(get_file_perms($dir,$item));
		if($allow) echo "</A>";
		echo "</TD>\n";
	// Actions
		echo "<TD>\n<TABLE>\n";
		// EDIT
		if(get_is_editable($dir, $item)) {
			if($allow) {
				echo "<TD><A HREF=\"".make_link("edit",$dir,$item)."\">";
				echo "<IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
				echo "src=\"_img/_edit.gif\" ALT=\"".$GLOBALS["messages"]["editlink"]."\" TITLE=\"";
				echo $GLOBALS["messages"]["editlink"]."\"></A></TD>\n";
			} else {
				echo "<TD><IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
				echo "src=\"_img/_edit_.gif\" ALT=\"".$GLOBALS["messages"]["editlink"]."\" TITLE=\"";
				echo $GLOBALS["messages"]["editlink"]."\"></TD>\n";
			}
		} else {
			echo "<TD><IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
			echo "src=\"_img/_.gif\" ALT=\"\"></TD>\n";
		}
		echo "</TABLE>\n</TD></TR>\n";
	}
}
//------------------------------------------------------------------------------
// MAIN FUNCTION
function list_dir($dir) {			// list directory contents
	$allow=($GLOBALS["permissions"]&01)==01;
	$admin=((($GLOBALS["permissions"]&04)==04) || (($GLOBALS["permissions"]&02)==02));
	
	$dir_up = dirname($dir);
	if($dir_up==".") $dir_up = "";
	
	if(!get_show_item($dir_up,basename($dir))) show_error($dir." : ".$GLOBALS["error_msg"]["accessdir"]);
	
	// make file & dir tables, & get total filesize & number of items
	make_tables($dir, $dir_list, $file_list, $tot_file_size, $num_items);
	
	$s_dir=$dir;		if(strlen($s_dir)>50) $s_dir="...".substr($s_dir,-47);
	show_header($GLOBALS["messages"]["actdir"].": /".get_rel_item("",$s_dir));
	
	// Javascript functions:
	include "./.include/javascript.php";
	
	// Sorting of items
	$_img = "&nbsp;<IMG width=\"10\" height=\"10\" border=\"0\" align=\"ABSMIDDLE\" src=\"_img/";
	if($GLOBALS["srt"]=="yes") {
		$_srt = "no";	$_img .= "_arrowup.gif\" ALT=\"^\">";
	} else {
		$_srt = "yes";	$_img .= "_arrowdown.gif\" ALT=\"v\">";
	}
	
	// Toolbar
	echo "<BR><TABLE width=\"95%\"><TR><TD><TABLE><TR>\n";
	
	// PARENT DIR
	echo "<TD><A HREF=\"".make_link("list",$dir_up,NULL)."\">";
	echo "<IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" src=\"_img/_up.gif\" ";
	echo "ALT=\"".$GLOBALS["messages"]["uplink"]."\" TITLE=\"".$GLOBALS["messages"]["uplink"]."\"></A></TD>\n";
	// HOME DIR
	echo "<TD><A HREF=\"".make_link("list",NULL,NULL)."\">";
	echo "<IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" src=\"_img/_home.gif\" ";
	echo "ALT=\"".$GLOBALS["messages"]["homelink"]."\" TITLE=\"".$GLOBALS["messages"]["homelink"]."\"></A></TD>\n";
	// RELOAD
	echo "<TD><A HREF=\"javascript:location.reload();\"><IMG border=\"0\" width=\"16\" height=\"16\" ";
	echo "align=\"ABSMIDDLE\" src=\"_img/_refresh.gif\" ALT=\"".$GLOBALS["messages"]["reloadlink"];
	echo "\" TITLE=\"".$GLOBALS["messages"]["reloadlink"]."\"></A></TD>\n";
	// SEARCH
	echo "<TD><A HREF=\"".make_link("search",$dir,NULL)."\">";
	echo "<IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" src=\"_img/_search.gif\" ";
	echo "ALT=\"".$GLOBALS["messages"]["searchlink"]."\" TITLE=\"".$GLOBALS["messages"]["searchlink"];
	echo "\"></A></TD>\n";
	
	if($allow) {
		// COPY
		echo "<TD><A HREF=\"javascript:Copy();\"><IMG border=\"0\" width=\"16\" height=\"16\" ";
		echo "align=\"ABSMIDDLE\" src=\"_img/_copy.gif\" ALT=\"".$GLOBALS["messages"]["copylink"];
		echo "\" TITLE=\"".$GLOBALS["messages"]["copylink"]."\"></A></TD>\n";
		// MOVE
		echo "<TD><A HREF=\"javascript:Move();\"><IMG border=\"0\" width=\"16\" height=\"16\" ";
		echo "align=\"ABSMIDDLE\" src=\"_img/_move.gif\" ALT=\"".$GLOBALS["messages"]["movelink"];
		echo "\" TITLE=\"".$GLOBALS["messages"]["movelink"]."\"></A></TD>\n";
		// DELETE
		echo "<TD><A HREF=\"javascript:Delete();\"><IMG border=\"0\" width=\"16\" height=\"16\" ";
		echo "align=\"ABSMIDDLE\" src=\"_img/_delete.gif\" ALT=\"".$GLOBALS["messages"]["dellink"];
		echo "\" TITLE=\"".$GLOBALS["messages"]["dellink"]."\"></A></TD>\n";
		// ARCHIVE
		if($GLOBALS["zip"] || $GLOBALS["tar"] || $GLOBALS["tgz"]) {
			echo "<TD><A HREF=\"javascript:Archive();\"><IMG border=\"0\" width=\"16\" height=\"16\" ";
			echo "align=\"ABSMIDDLE\" src=\"_img/_archive.gif\" ALT=\"".$GLOBALS["messages"]["comprlink"];
			echo "\" TITLE=\"".$GLOBALS["messages"]["comprlink"]."\"></A></TD>\n";
		}
	} else {
		// COPY
		echo "<TD><IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
		echo "src=\"_img/_copy_.gif\" ALT=\"".$GLOBALS["messages"]["copylink"]."\" TITLE=\"";
		echo $GLOBALS["messages"]["copylink"]."\"></TD>\n";
		// MOVE
		echo "<TD><IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
		echo "src=\"_img/_move_.gif\" ALT=\"".$GLOBALS["messages"]["movelink"]."\" TITLE=\"";
		echo $GLOBALS["messages"]["movelink"]."\"></TD>\n";
		// DELETE
		echo "<TD><IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
		echo "src=\"_img/_delete_.gif\" ALT=\"".$GLOBALS["messages"]["dellink"]."\" TITLE=\"";
		echo $GLOBALS["messages"]["dellink"]."\"></TD>\n";
		// UPLOAD
		echo "<TD><IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
		echo "src=\"_img/_upload_.gif\" ALT=\"".$GLOBALS["messages"]["uplink"];
		echo "\" TITLE=\"".$GLOBALS["messages"]["uplink"]."\"></TD>\n";
	}
	
	// ADMIN & LOGOUT
	if($GLOBALS["require_login"]) {
		// ADMIN
		if($admin) {
			echo "<TD><A HREF=\"".make_link("admin",$dir,NULL)."\">";
			echo "<IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
			echo "src=\"_img/_admin.gif\" ALT=\"".$GLOBALS["messages"]["adminlink"]."\" TITLE=\"";
			echo $GLOBALS["messages"]["adminlink"]."\"></A></TD>\n";
		}
		// LOGOUT
		echo "<TD><A HREF=\"".make_link("logout",NULL,NULL)."\">";
		echo "<IMG border=\"0\" width=\"16\" height=\"16\" align=\"ABSMIDDLE\" ";
		echo "src=\"_img/_logout.gif\" ALT=\"".$GLOBALS["messages"]["logoutlink"]."\" TITLE=\"";
		echo $GLOBALS["messages"]["logoutlink"]."\"></A></TD>\n";
	}
	echo "</TR></TABLE></TD>\n";
	
	// Create File / Dir
	if($allow) {
		echo "<TD align=\"right\"><TABLE><FORM action=\"".make_link("mkitem",$dir,NULL)."\" method=\"post\">\n<TR><TD>";
		echo "<SELECT name=\"mktype\"><option value=\"file\">".$GLOBALS["mimes"]["file"]."</option>";
		echo "<option value=\"dir\">".$GLOBALS["mimes"]["dir"]."</option></SELECT>\n";
		echo "<INPUT name=\"mkname\" type=\"text\" size=\"15\">";
		echo "<INPUT type=\"submit\" value=\"".$GLOBALS["messages"]["btncreate"];
		echo "\"></TD></TR></FORM></TABLE></TD>\n";
	}
	
	echo "</TR></TABLE>\n";
	
	// End Toolbar
	
	
	// Begin Table + Form for checkboxes
	echo"<TABLE WIDTH=\"95%\"><FORM name=\"selform\" method=\"POST\" action=\"".make_link("post",$dir,NULL)."\">\n";
	echo "<INPUT type=\"hidden\" name=\"do_action\"><INPUT type=\"hidden\" name=\"first\" value=\"y\">\n";
	
	// Table Header
	echo "<TR><TD colspan=\"7\"><HR></TD></TR><TR><TD WIDTH=\"2%\" class=\"header\">\n";
	echo "<INPUT TYPE=\"checkbox\" name=\"toggleAllC\" onclick=\"javascript:ToggleAll(this);\"></TD>\n";
	echo "<TD WIDTH=\"44%\" class=\"header\"><B>\n";
	if($GLOBALS["order"]=="name") $new_srt = $_srt;	else $new_srt = "yes";
	echo "<A href=\"".make_link("list",$dir,NULL,"name",$new_srt)."\">".$GLOBALS["messages"]["nameheader"];
	if($GLOBALS["order"]=="name") echo $_img;
	echo "</A></B></TD>\n<TD WIDTH=\"10%\" class=\"header\"><B>";
	if($GLOBALS["order"]=="size") $new_srt = $_srt;	else $new_srt = "yes";
	echo "<A href=\"".make_link("list",$dir,NULL,"size",$new_srt)."\">".$GLOBALS["messages"]["sizeheader"];
	if($GLOBALS["order"]=="size") echo $_img;
	echo "</A></B></TD>\n<TD WIDTH=\"16%\" class=\"header\"><B>";
	if($GLOBALS["order"]=="type") $new_srt = $_srt;	else $new_srt = "yes";
	echo "<A href=\"".make_link("list",$dir,NULL,"type",$new_srt)."\">".$GLOBALS["messages"]["typeheader"];
	if($GLOBALS["order"]=="type") echo $_img;
	echo "</A></B></TD>\n<TD WIDTH=\"14%\" class=\"header\"><B>";
	if($GLOBALS["order"]=="mod") $new_srt = $_srt;	else $new_srt = "yes";
	echo "<A href=\"".make_link("list",$dir,NULL,"mod",$new_srt)."\">".$GLOBALS["messages"]["modifheader"];
	if($GLOBALS["order"]=="mod") echo $_img;
	echo "</A></B></TD><TD WIDTH=\"8%\" class=\"header\"><B>".$GLOBALS["messages"]["permheader"]."</B>\n";
	echo "</TD><TD WIDTH=\"6%\" class=\"header\"><B>".$GLOBALS["messages"]["actionheader"]."</B></TD></TR>\n";
	echo "<TR><TD colspan=\"7\"><HR></TD></TR>\n";
		
	// make & print Table using lists
	print_table($dir, make_list($dir_list, $file_list), $allow);

	// print number of items & total filesize
	echo "<TR><TD colspan=\"7\"><HR></TD></TR><TR>\n<TD class=\"header\"></TD>";
	echo "<TD class=\"header\">".$num_items." ".$GLOBALS["messages"]["miscitems"]." (";
	if(function_exists("disk_free_space")) {
		$free=parse_file_size(disk_free_space(get_abs_dir($dir)));
	} elseif(function_exists("diskfreespace")) {
		$free=parse_file_size(diskfreespace(get_abs_dir($dir)));
	} else $free="?";
	// echo "Total: ".parse_file_size(disk_total_space(get_abs_dir($dir))).", ";
	echo $GLOBALS["messages"]["miscfree"].": ".$free.")</TD>\n";
	echo "<TD class=\"header\">".parse_file_size($tot_file_size)."</TD>\n";
	for($i=0;$i<4;++$i) echo"<TD class=\"header\"></TD>";
	echo "</TR>\n<TR><TD colspan=\"7\"><HR></TD></TR></FORM></TABLE>\n";
	
?><script language="JavaScript1.2" type="text/javascript">
<!--
	// Uncheck all items (to avoid problems with new items)
	var ml = document.selform;
	var len = ml.elements.length;
	for(var i=0; i<len; ++i) {
		var e = ml.elements[i];
		if(e.name == "selitems[]" && e.checked == true) {
			e.checked=false;
		}
	}
// -->
</script><?php
}
//------------------------------------------------------------------------------
?>
