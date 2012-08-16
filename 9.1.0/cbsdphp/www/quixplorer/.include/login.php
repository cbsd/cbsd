<?php
/*
	login.php
	
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
	User Authentication Functions
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
require "./.include/users.php";
load_users();
//------------------------------------------------------------------------------
session_start();
if(isset($_SESSION)) 			$GLOBALS['__SESSION']=&$_SESSION;
elseif(isset($HTTP_SESSION_VARS))	$GLOBALS['__SESSION']=&$HTTP_SESSION_VARS;
else logout();
//------------------------------------------------------------------------------
function login() {
	//print_r($GLOBALS['__SESSION']);	
	if(isset($GLOBALS['__SESSION']["s_user"])) {
		if(!activate_user($GLOBALS['__SESSION']["s_user"],$GLOBALS['__SESSION']["s_pass"])) {
			logout();
		}
		$GLOBALS["lang"] = $GLOBALS['__SESSION']["s_lang"];
		$GLOBALS["language"] = $GLOBALS['__SESSION']["s_lang"];
		require "./_lang/".$GLOBALS["language"].".php";
		require "./_lang/".$GLOBALS["language"]."_mimes.php";
	} else {
		if(isset($GLOBALS['__POST']["p_pass"])) $p_pass=$GLOBALS['__POST']["p_pass"];
		else $p_pass="";
		
		if(isset($GLOBALS['__POST']["p_user"])) {
			// Check Login
			if(!activate_user(stripslashes($GLOBALS['__POST']["p_user"]), md5(stripslashes($p_pass)))) {
				logout();
			}
			$GLOBALS['__SESSION']["s_lang"] = $GLOBALS['__POST']["lang"];
			return;
		} else {
			// Ask for Login
			show_header($GLOBALS["messages"]["actlogin"]);
			echo "<CENTER><BR><TABLE width=\"300\"><TR><TD colspan=\"2\" class=\"header\" nowrap><B>";
			echo $GLOBALS["messages"]["actloginheader"]."</B></TD></TR>\n<FORM name=\"login\" action=\"";
			echo make_link("login",NULL,NULL)."\" method=\"post\">\n";
			echo "<TR><TD>".$GLOBALS["messages"]["miscusername"].":</TD><TD align=\"right\">";
			echo "<INPUT name=\"p_user\" type=\"text\" size=\"25\"></TD></TR>\n";
			echo "<TR><TD>".$GLOBALS["messages"]["miscpassword"].":</TD><TD align=\"right\">";
			echo "<INPUT name=\"p_pass\" type=\"password\" size=\"25\"></TD></TR>\n";
			echo "<TR><TD>".gettext("Detected Language:<br />(Change if needed)")."</TD><TD align=\"right\">";
			//Select box and auto language detection array
			include('./_lang/_info.php');
			echo "<TR><TD colspan=\"2\" align=\"right\"><INPUT type=\"submit\" value=\"";
			echo $GLOBALS["messages"]["btnlogin"]."\"></TD></TR>\n</FORM></TABLE><BR></CENTER>\n";
?><script language="JavaScript1.2" type="text/javascript">
<!--
	if(document.login) document.login.p_user.focus();
// -->
</script><?php
			show_footer();
			exit;
		}
	}
}
//------------------------------------------------------------------------------
function logout() {
	$GLOBALS['__SESSION']=array();
	session_destroy();
	header("location: ".$GLOBALS["script_name"]);
}
//------------------------------------------------------------------------------
?>
