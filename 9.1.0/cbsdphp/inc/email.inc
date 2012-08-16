<?php
/*
	email.inc
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard (olivier@freenas.org).
	All rights reserved.

	Portions of m0n0wall (http://m0n0.ch/wall).
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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
require_once("config.inc");
require_once("util.inc");
require_once("system.inc");

// Checks if Email settings are valid.
// Return 0 if successful, otherwise 1.
function email_validate_settings() {
	global $config;

	$email = $config['system']['email'];

	if (empty($email['server']) || empty($email['port']) || empty($email['security']) || empty($email['from']))
		return 1;

	if (isset($email['auth']) && (empty($email['username']) || empty($email['password'])))
		return 1;

	return 0;
}

// Send an email.
// $address - The email adress
// $subject - The email subject
// $message - The message
// $error - Contains an error message if submission fails.
// Return 0 if successful, otherwise 1.
function email_send($address, $subject, $message, &$error) {
	global $config;

	$msgfilepath = "/tmp/message";

	// Get date in RFC 2882 format.
	$rfcdate = date('r');

	// Substitute special characters in subject.
	$subject = str_replace("%d", trim(shell_exec("date")), $subject);
	$subject = str_replace("%h", system_get_hostname(), $subject); 

	// Convert adresses for msmtp.
	$address = strtr($address, array(";" => " ", "," => " "));

	// Generate message content.
	$data = "";
	foreach (explode(" ", $address) as $addressv) {
		$data .= "To: {$addressv}\n";
	}
	$data .= <<<EOD
From: {$config['system']['email']['from']}
Subject: {$subject}
Date: {$rfcdate}

{$message}

EOD;

	// Write message to file.
	if (false === file_put_contents($msgfilepath, $data)) {
		write_log("Error: Failed to create '{$msgfilepath}'.\n");
		return 1;
	}

	// Send email via external SMTP client.
	mwexec2("/usr/local/bin/msmtp --file=/var/etc/msmtp.conf {$address} < {$msgfilepath}", $output, $retval);
	if (0 !== $retval) {
		$error = $output;
	}

	// Cleanup message.
	unlink($msgfilepath);

	return $retval;
}
?>
