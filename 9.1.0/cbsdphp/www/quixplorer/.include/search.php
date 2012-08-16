<?php
/*
	search.php
	
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
	File-Search Functions
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
function find_item($dir,$pat,&$list,$recur) {	// find items
	$handle=@opendir(get_abs_dir($dir));
	if($handle===false) return;		// unable to open dir
	
	while(($new_item=readdir($handle))!==false) {
		if(!@file_exists(get_abs_item($dir, $new_item))) continue;
		if(!get_show_item($dir, $new_item)) continue;
		
		// match?
		if(@eregi($pat,$new_item)) $list[]=array($dir,$new_item);
		
		// search sub-directories
		if(get_is_dir($dir, $new_item) && $recur) {
			find_item(get_rel_item($dir,$new_item),$pat,$list,$recur);
		}
	}
	
	closedir($handle);
}
//------------------------------------------------------------------------------
function make_list($dir,$item,$subdir) {	// make list of found items
	// convert shell-wildcards to PCRE Regex Syntax
	$pat="^".str_replace("?",".",str_replace("*",".*",str_replace(".","\.",$item)))."$";
	
	// search
	find_item($dir,$pat,$list,$subdir);
	if(is_array($list)) sort($list);
	return $list;
}
//------------------------------------------------------------------------------
function print_table($list) {			// print table of found items
	if(!is_array($list)) return;
	
	$cnt = count($list);
	for($i=0;$i<$cnt;++$i) {
		$dir = $list[$i][0];	$item = $list[$i][1];
		$s_dir=$dir;	if(strlen($s_dir)>65) $s_dir=substr($s_dir,0,62)."...";
		$s_item=$item;	if(strlen($s_item)>45) $s_item=substr($s_item,0,42)."...";
		$link = "";	$target = "";
		
		if(get_is_dir($dir,$item)) {
			$img = "dir.gif";
			$link = make_link("list",get_rel_item($dir, $item),NULL);
		} else {
			$img = get_mime_type($dir, $item, "img");
			//if(get_is_editable($dir,$item) || get_is_image($dir,$item)) {
				$link = $GLOBALS["home_url"]."/".get_rel_item($dir, $item);
				$target = "_blank";
			//}
		}
		
		echo "<CENTER><TR><TD>" . "<IMG border=\"0\" width=\"16\" height=\"16\" ";
		echo "align=\"ABSMIDDLE\" src=\"_img/" . $img . "\" ALT=\"\">&nbsp;";
		/*if($link!="")*/ echo"<A HREF=\"".$link."\" TARGET=\"".$target."\">";
		//else echo "<A>";
		echo $s_item."</A></TD><TD><A HREF=\"" . make_link("list",$dir,NULL)."\"> /";
		echo $s_dir."</A></TD></TR></CENTER>\n";
	}
}
//------------------------------------------------------------------------------
function search_items($dir) {			// search for item
	if(isset($GLOBALS['__POST']["searchitem"])) {
		$searchitem=stripslashes($GLOBALS['__POST']["searchitem"]);
		$subdir=(isset($GLOBALS['__POST']["subdir"]) && $GLOBALS['__POST']["subdir"]=="y");
		$list=make_list($dir,$searchitem,$subdir);
	} else {
		$searchitem=NULL;
		$subdir=true;
	}
	
	$msg=$GLOBALS["messages"]["actsearchresults"];
	if($searchitem!=NULL) $msg.=": (/" . get_rel_item($dir, $searchitem).")";
	show_header($msg);
	
	// Search Box
	echo "<CENTER><BR><TABLE><FORM name=\"searchform\" action=\"".make_link("search",$dir,NULL);
	echo "\" method=\"post\">\n<TR><TD><INPUT name=\"searchitem\" type=\"text\" size=\"25\" value=\"";
	echo $searchitem."\"><INPUT type=\"submit\" value=\"".$GLOBALS["messages"]["btnsearch"];
	echo "\">&nbsp;<input type=\"button\" value=\"".$GLOBALS["messages"]["btnclose"];
	echo "\" onClick=\"javascript:location='".make_link("list",$dir,NULL);
	echo "';\"></TD></TR><TR><TD><INPUT type=\"checkbox\" name=\"subdir\" value=\"y\"";
	echo ($subdir?" checked>":">").$GLOBALS["messages"]["miscsubdirs"]."</TD></TR></FORM></TABLE></CENTER>\n";
	
	// Results
	if($searchitem!=NULL) {
		echo "<TABLE width=\"95%\"><TR><TD colspan=\"2\"><HR></TD></TR>\n";
		if(count($list)>0) {
			// Table Header
			echo "<TR>\n<TD WIDTH=\"42%\" class=\"header\"><B>".$GLOBALS["messages"]["nameheader"];
			echo "</B></TD>\n<TD WIDTH=\"58%\" class=\"header\"><B>".$GLOBALS["messages"]["pathheader"];
			echo "</B></TD></TR>\n<TR><TD colspan=\"2\"><HR></TD></TR>\n";
	
			// make & print table of found items
			print_table($list);

			echo "<TR><TD colspan=\"2\"><HR></TD></TR>\n<TR><TD class=\"header\">".count($list)." ";
			echo $GLOBALS["messages"]["miscitems"].".</TD><TD class=\"header\"></TD></TR>\n";
		} else {
			echo "<TR><TD>".$GLOBALS["messages"]["miscnoresult"]."</TD></TR>";
		}
		echo "<TR><TD colspan=\"2\"><HR></TD></TR></TABLE>\n";
	}
?><script language="JavaScript1.2" type="text/javascript">
<!--
	if(document.searchform) document.searchform.searchitem.focus();
// -->
</script><?php
}
//------------------------------------------------------------------------------
?>