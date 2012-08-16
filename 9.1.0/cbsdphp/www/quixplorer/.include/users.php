<?php
/*
	users.php
	
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
	Administrative Functions
------------------------------------------------------------------------------*/
//------------------------------------------------------------------------------
function load_users() {
	require "./.config/.htusers.php";
}
//------------------------------------------------------------------------------
function save_users() {
	$cnt=count($GLOBALS["users"]);
	if($cnt>0) sort($GLOBALS["users"]);
	
	// Make PHP-File
	$content='<?php $GLOBALS["users"]=array(';
	for($i=0;$i<$cnt;++$i) {
		// if($GLOBALS["users"][6]&4==4) $GLOBALS["users"][6]=7;	// If admin, all permissions
		$content.="\r\n\tarray(\"".$GLOBALS["users"][$i][0].'","'.
			$GLOBALS["users"][$i][1].'","'.$GLOBALS["users"][$i][2].'","'.$GLOBALS["users"][$i][3].'",'.
			$GLOBALS["users"][$i][4].',"'.$GLOBALS["users"][$i][5].'",'.$GLOBALS["users"][$i][6].','.
			$GLOBALS["users"][$i][7].'),';
	}
	$content.="\r\n); ?>";
	
	// Write to File
	$fp = @fopen("./.config/.htusers.php", "w");
	if($fp===false) return false;	// Error
	fputs($fp,$content);
	fclose($fp);
	
	return true;
}
//------------------------------------------------------------------------------
function &find_user($user,$pass) {
	$cnt=count($GLOBALS["users"]);
	for($i=0;$i<$cnt;++$i) {
		if($user==$GLOBALS["users"][$i][0]) {
			if($pass==NULL || ($pass==$GLOBALS["users"][$i][1] &&
				$GLOBALS["users"][$i][7]))
			{
				return $GLOBALS["users"][$i];
			}
		}
	}
	
	return NULL;
}
//------------------------------------------------------------------------------
function activate_user($user,$pass) {
	$data=find_user($user,$pass);
	if($data==NULL) return false;
	
	// Set Login
	$GLOBALS['__SESSION']["s_user"]	= $data[0];
	$GLOBALS['__SESSION']["s_pass"]	= $data[1];
	$GLOBALS["home_dir"]	= $data[2];
	$GLOBALS["home_url"]	= $data[3];
	$GLOBALS["show_hidden"]	= $data[4];
	$GLOBALS["no_access"]	= $data[5];
	$GLOBALS["permissions"]	= $data[6];
	
	return true;
}
//------------------------------------------------------------------------------
function update_user($user,$new_data) {
	$data=&find_user($user,NULL);
	if($data==NULL) return false;
	
	$data=$new_data;
	return save_users();
}
//------------------------------------------------------------------------------
function add_user($data) {
	if(find_user($data[0],NULL)) return false;
	
	$GLOBALS["users"][]=$data;
	return save_users();
}
//------------------------------------------------------------------------------
function remove_user($user) {
	$data=&find_user($user,NULL);
	if($data==NULL) return false;
	
	// Remove
	$data=NULL;
	
	// Copy Valid Users
	$cnt=count($GLOBALS["users"]);
	for($i=0;$i<$cnt;++$i) {
		if($GLOBALS["users"][$i]!=NULL) $save_users[]=$GLOBALS["users"][$i];
	}
	$GLOBALS["users"]=$save_users;
	return save_users();
}
//------------------------------------------------------------------------------
/*
function num_users($active=true) {
	$cnt=count($GLOBALS["users"]);
	if(!$active) return $cnt;
	
	for($i=0, $j=0;$i<$cnt;++$i) {
		if($GLOBALS["users"][$i][7]) ++$j;
	}
	return $j;
}
*/
//------------------------------------------------------------------------------
?>