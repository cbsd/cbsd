<?php
/*
	init.php
	
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
	Main File
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
// Vars
if(isset($_SERVER)) {
	$GLOBALS['__GET']	=&$_GET;
	$GLOBALS['__POST']	=&$_POST;
	$GLOBALS['__SERVER']	=&$_SERVER;
	$GLOBALS['__FILES']	=&$_FILES;
} elseif(isset($HTTP_SERVER_VARS)) {
	$GLOBALS['__GET']	=&$HTTP_GET_VARS;
	$GLOBALS['__POST']	=&$HTTP_POST_VARS;
	$GLOBALS['__SERVER']	=&$HTTP_SERVER_VARS;
	$GLOBALS['__FILES']	=&$HTTP_POST_FILES;
} else {
	die("<B>ERROR: Your PHP version is too old</B><BR>".
	"You need at least PHP 4.0.0 to run QuiXplorer; preferably PHP 4.3.1 or higher.");
}
//------------------------------------------------------------------------------
// Get Action
if(isset($GLOBALS['__GET']["action"])) $GLOBALS["action"]=$GLOBALS['__GET']["action"];
else $GLOBALS["action"]="list";
if($GLOBALS["action"]=="post" && isset($GLOBALS['__POST']["do_action"])) {
	$GLOBALS["action"]=$GLOBALS['__POST']["do_action"];
}
if($GLOBALS["action"]=="") $GLOBALS["action"]="list";
$GLOBALS["action"]=stripslashes($GLOBALS["action"]);
// Default Dir
if(isset($GLOBALS['__GET']["dir"])) $GLOBALS["dir"]=stripslashes($GLOBALS['__GET']["dir"]);
else $GLOBALS["dir"]="";
if($GLOBALS["dir"]==".") $GLOBALS["dir"]=="";
// Get Item
if(isset($GLOBALS['__GET']["item"])) $GLOBALS["item"]=stripslashes($GLOBALS['__GET']["item"]);
else $GLOBALS["item"]="";
// Get Sort
if(isset($GLOBALS['__GET']["order"])) $GLOBALS["order"]=stripslashes($GLOBALS['__GET']["order"]);
else $GLOBALS["order"]="name";
if($GLOBALS["order"]=="") $GLOBALS["order"]=="name";
// Get Sortorder (yes==up)
if(isset($GLOBALS['__GET']["srt"])) $GLOBALS["srt"]=stripslashes($GLOBALS['__GET']["srt"]);
else $GLOBALS["srt"]="yes";
if($GLOBALS["srt"]=="") $GLOBALS["srt"]=="yes";
// Get Language
if(isset($GLOBALS['__GET']["lang"])) $GLOBALS["lang"]=basename($GLOBALS['__GET']["lang"]);
elseif(isset($GLOBALS['__POST']["lang"])) $GLOBALS["lang"]=basename($GLOBALS['__POST']["lang"]); 
//------------------------------------------------------------------------------
// Necessary files
ob_start(); // prevent unwanted output
require "./.config/conf.php";
if(isset($GLOBALS["lang"])) $GLOBALS["language"]=$GLOBALS["lang"];
if(file_exists("./_lang/".$GLOBALS["language"].".php")) require "./_lang/".$GLOBALS["language"].".php";
else require "./_lang/en.php";
if(file_exists("./_lang/".$GLOBALS["language"]."_mimes.php")) require "./_lang/".$GLOBALS["language"]."_mimes.php";
else require "./_lang/en_mimes.php"; 
require "./.config/mimes.php";
require "./.include/extra.php";
require "./.include/header.php";
require "./.include/footer.php";
require "./.include/error.php";
$tmp_msg = $GLOBALS["login_prompt"][$GLOBALS["language"]];
if (isset($tmp_msg))
	$GLOBALS["messages"]["actloginheader"] = $tmp_msg;

ob_end_clean(); // get rid of cached unwanted output
//------------------------------------------------------------------------------
if($GLOBALS["require_login"]) {	// LOGIN
	ob_start(); // prevent unwanted output
	require "./.include/login.php";
	ob_end_clean(); // get rid of cached unwanted output
	if($GLOBALS["action"]=="logout") {
		logout();
	} else {
		login();
	}
}
//------------------------------------------------------------------------------
$abs_dir=get_abs_dir($GLOBALS["dir"]);
if(!@file_exists($GLOBALS["home_dir"])) {
	if($GLOBALS["require_login"]) {
		$extra="<A HREF=\"".make_link("logout",NULL,NULL)."\">".
			$GLOBALS["messages"]["btnlogout"]."</A>";
	} else $extra=NULL;
	show_error($GLOBALS["error_msg"]["home"],$extra);
}
if(!down_home($abs_dir)) show_error($GLOBALS["dir"]." : ".$GLOBALS["error_msg"]["abovehome"]);
if(!is_dir($abs_dir)) show_error($GLOBALS["dir"]." : ".$GLOBALS["error_msg"]["direxist"]);
//------------------------------------------------------------------------------
?>
