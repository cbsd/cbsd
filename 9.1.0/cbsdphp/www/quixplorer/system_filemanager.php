<?php
/*
	system_filemanager.php
	
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
umask(002); // Added to make created files/dirs group writable
//------------------------------------------------------------------------------
require "./.include/init.php";	// Init
//------------------------------------------------------------------------------
switch($GLOBALS["action"]) {		// Execute action
//------------------------------------------------------------------------------
// EDIT FILE
case "edit":
	require "./.include/edit.php";
	edit_file($GLOBALS["dir"], $GLOBALS["item"]);
break;
//------------------------------------------------------------------------------
// DELETE FILE(S)/DIR(S)
case "delete":
	require "./.include/del.php";
	del_items($GLOBALS["dir"]);
break;
//------------------------------------------------------------------------------
// COPY/MOVE FILE(S)/DIR(S)
case "copy":	case "move":
	require "./.include/copy_move.php";
	copy_move_items($GLOBALS["dir"]);
break;
//------------------------------------------------------------------------------
// CREATE DIR/FILE
case "mkitem":
	require "./.include/mkitem.php";
	make_item($GLOBALS["dir"]);
break;
//------------------------------------------------------------------------------
// CHMOD FILE/DIR
case "chmod":
	require "./.include/chmod.php";
	chmod_item($GLOBALS["dir"], $GLOBALS["item"]);
break;
//------------------------------------------------------------------------------
// SEARCH FOR FILE(S)/DIR(S)
case "search":
	require "./.include/search.php";
	search_items($GLOBALS["dir"]);
break;
//------------------------------------------------------------------------------
// CREATE ARCHIVE
case "arch":
	require "./.include/archive.php";
	archive_items($GLOBALS["dir"]);
break;
//------------------------------------------------------------------------------
// USER-ADMINISTRATION
case "admin":
	require "./.include/admin.php";
	show_admin($GLOBALS["dir"]);
break;
//------------------------------------------------------------------------------
// DEFAULT: LIST FILES & DIRS
case "list":
default:
	require "./.include/list.php";
	list_dir($GLOBALS["dir"]);
//------------------------------------------------------------------------------
}				// end switch-statement
//------------------------------------------------------------------------------
echo "<br /><br /><br /><br />\n";
show_footer();
//------------------------------------------------------------------------------
?>
