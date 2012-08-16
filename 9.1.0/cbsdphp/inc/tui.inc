<?php
/*
	tui.inc
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard (olivier@freenas.org).
	All rights reserved.

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
require_once("util.inc");

// Display a yes|no dialog.
// $text - The dialog text
// $width - Width of the dialog
// $height - Height of the dialog
// Returns 0 if yes, 1 if no or 255 if aborted via ESC.
function tui_display_yesno($text, $width, $height, $defaultyes = true) {
	$param = "";
	if (false === $defaultyes) $param = "--defaultno";
	passthru("/usr/local/bin/cdialog --clear {$param} --yesno \"{$text}\" {$height} {$width}", $retval);
	return $retval;
}

// Display a message box.
// $title - The dialog title
// $text - The dialog text
// $width - Width of the dialog
// $height - Height of the dialog
// Returns 0 if ok has been pressed or 255 if aborted via ESC.
function tui_display_message($title, $text, $width, $height) {
	passthru("/usr/local/bin/cdialog --clear --title \"{$title}\" --msgbox \"{$text}\" {$height} {$width}", $retval);
	return $retval;
}

// Display a message box.
// $title - The dialog title
// $text - The dialog text
// $width - Width of the dialog
// $height - Height of the dialog
// Returns 0 if OK, 1 if Cancel or 255 if aborted via ESC.
function tui_display_message_ex($title, $text, $width, $height) {
	passthru("/usr/local/bin/cdialog --clear --yes-label \"OK\" --no-label \"Cancel\" --title \"{$title}\" --yesno \"{$text}\" {$height} {$width}", $retval);
	return $retval;
}

// Display a input box.
// $text - The dialog text
// $width - Width of the dialog
// $height - Height of the dialog
// $value [in|out]- Initial value. Also contains result.
// Returns 0 if ok has been pressed or 255 if aborted via ESC.
function tui_display_input($text, $width, $height, &$value) {
	$tmpfile = "/tmp/tui" . rand();
	passthru("/usr/local/bin/cdialog --input-fd 2 --output-fd 1 --clear --inputbox \"{$text}\" {$height} {$width} \"{$value}\" 1>{$tmpfile}", $retval);
	$value = file_get_contents($tmpfile);
	unlink($tmpfile);
	return $retval;
}

// Display a menu.
// $title - The dialog title
// $menutitle - The menu title
// $width - Width of the dialog
// $height - Height of the dialog
// $menuheight - Height of the menu
// $amenuitem - The menu item array => Array
// (
// 	[tag]  => "x"
// 	[item] => "xy zxy yxz"
// )
// $value [out] - Contains the selected item tag.
// Returns 0 if ok has been pressed or 255 if aborted via ESC.
function tui_display_menu($title, $menutitle, $width, $height, $menuheight, $amenuitem, &$value) {
	$tmpfile = "/tmp/tui" . rand();
	$cmd = "/usr/local/bin/cdialog --input-fd 2 --output-fd 1 --clear --title \"{$title}\" --menu \"{$menutitle}\" {$height} {$width} {$menuheight}";
	foreach($amenuitem as $menuitemv) {
		$cmd .= " \"{$menuitemv['tag']}\" \"{$menuitemv['item']}\"";
	}
	$cmd .= " 1>{$tmpfile}";
	passthru($cmd, $retval);
	$value = file_get_contents($tmpfile);
	unlink($tmpfile);
	return $retval;
}

// Wait until user has pressed 'RETURN'.
function tui_wait_keypress($message = "") {
	if (!empty($message))
		write_console("$message\n");
	write_console("\nPress ENTER to continue.");
	$fp = fopen('php://stdin', 'r');
	fgets($fp);
	fclose($fp);
}
?>
