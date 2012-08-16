<?php
/*
	updatenotify.inc
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
require_once("globals.inc");

// Update notification constants
define("UPDATENOTIFY_MODE_NEW", 0);
define("UPDATENOTIFY_MODE_MODIFIED", 1);
define("UPDATENOTIFY_MODE_DIRTY", 2);
define("UPDATENOTIFY_MODE_UNKNOWN", 3);

define("UPDATENOTIFY_DB", "{$g['vardb_path']}/updatenotify.db");

// Create database object and create tables if necessary.
try {
	$createdb = !file_exists(UPDATENOTIFY_DB);
	$dbh = new PDO('sqlite:'.UPDATENOTIFY_DB);
	if ($createdb) {
		updatenotify_initdb();
	}
} catch (PDOException $e) {
	print $e->getMessage();
}

// Initialize the database by creating the tables.
// Return TRUE if successful, otherwise FALSE.
function updatenotify_initdb() {
	global $dbh;

	try {
		$dbh->exec("CREATE TABLE updatenotify (id INTEGER PRIMARY KEY, key TEXT, ts INTEGER, mode INTEGER, data TEXT)");
	} catch (PDOException $e) {
		print $e->getMessage();
		return FALSE;
	}

	return TRUE;
}

// Set a notification.
// $key - Key to be used
// $mode - Mode to be set
// $data - Data to be set
// Return TRUE if successful, otherwise FALSE.
function updatenotify_set($key, $mode, $data) {
	global $dbh;

	try {
		$sql = "INSERT INTO updatenotify (key, ts, mode, data) VALUES (:key, :ts, :mode, :data)";
		$stmt = $dbh->prepare($sql);
		$stmt->execute(array(':key' => $key, ':ts' => time(), ':mode' => $mode, ':data' => $data));
		$stmt = null;
	} catch (PDOException $e) {
		print $e->getMessage();
		return FALSE;
	}

	return TRUE;
}

// Get all notifications for the given key.
// $key - Key to be processed
// Return array of notifications, otherwise FALSE.
function updatenotify_get($key) {
	global $dbh;

	try {
		$sql = "SELECT mode, data FROM updatenotify WHERE key = :key ORDER BY ts DESC";
		$stmt = $dbh->prepare($sql);
		$stmt->execute(array(':key' => $key));

		$result = array();
		while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
			$result[] = $row;
		}

		$stmt = null;
	} catch (PDOException $e) {
		print $e->getMessage();
		return FALSE;
	}

	return $result;
}

// Remove notifications with given key.
// $key - Key to be removed
// Return TRUE if successful, otherwise FALSE.
function updatenotify_delete($key) {
	global $dbh;

	try {
		$sql = "DELETE FROM updatenotify WHERE key = :key";
		$stmt = $dbh->prepare($sql);
		$stmt->execute(array(':key' => $key));
		$stmt = null;
	} catch (PDOException $e) {
		print $e->getMessage();
		return FALSE;
	}

	return TRUE;
}

// Get the notification mode.
// $key - File to be used
// $data - Data to searched for
// Returns the notification mode, otherwise UPDATENOTIFY_MODE_UNKNOWN
function updatenotify_get_mode($key, $data, $matchwholeword = TRUE) {
	global $dbh;

	try {
		if (true === $matchwholeword)
			$ssql = "data = :data";
		else
			$ssql = "data LIKE '%:data%'";

		// Remove duplicate entries. Last inserted notification wins (last in, first out).
		$sql = "SELECT mode FROM updatenotify WHERE key = :key AND {$ssql} AND id IN (SELECT id FROM (SELECT id, MAX(ts) AS ts FROM updatenotify GROUP BY data))";
		$stmt = $dbh->prepare($sql);
		$stmt->execute(array(':key' => $key, ':data' => $data));

		$result = $stmt->fetch(PDO::FETCH_ASSOC);
		if (FALSE !== $result)
			$result = $result['mode'];
		else
			$result = UPDATENOTIFY_MODE_UNKNOWN;

		$stmt = null;
	} catch (PDOException $e) {
		print $e->getMessage();
		return UPDATENOTIFY_MODE_UNKNOWN;
	}

	return $result;
}

// Check if any notification is set for the given key.
// $key - Key to be checked
// Return true if successful, otherwise false.
function updatenotify_exists($key) {
	global $dbh;

	try {
		$sql = "SELECT COUNT(*) FROM updatenotify WHERE key = :key";
		$stmt = $dbh->prepare($sql);
		$stmt->execute(array(':key' => $key));

		$result = $stmt->fetchColumn();
		$result = (0 < $result) ? TRUE : FALSE;

		$stmt = null;
	} catch (PDOException $e) {
		print $e->getMessage();
		return FALSE;
	}

	return $result;
}

// Check if specified notification mode is set.
// $key - Key to be checked
// $mode - Mode to searched for
// Return TRUE if successful, otherwise FALSE.
function updatenotify_exists_mode($key, $mode) {
	global $dbh;

	try {
		// Remove duplicate entries. Last inserted notification wins (last in, first out).
		$sql = "SELECT COUNT(*) FROM updatenotify WHERE key = :key AND mode = :mode AND id IN (SELECT id FROM (SELECT id, MAX(ts) AS ts FROM updatenotify GROUP BY mode))";
		$stmt = $dbh->prepare($sql);
		$stmt->execute(array(':key' => $key, ':mode' => $mode));

		$result = $stmt->fetchColumn();
		$result = (0 < $result) ? TRUE : FALSE;

		$stmt = null;
	} catch (PDOException $e) {
		print $e->getMessage();
		return FALSE;
	}

	return $result;
}

// Process notifications.
// $key - Key to be processed
// $function - Name of callback function. Function syntax: xyz($mode, $data)
// Return 0 if successful, otherwise 1.
function updatenotify_process($key, $function) {
	$a_notification = updatenotify_get($key);
	if (!is_array($a_notification))
		return 0;

	foreach ($a_notification as $notificationv) {
		if (0 != call_user_func($function, $notificationv['mode'], $notificationv['data']))
			return 1;
	}

	return 0;
}
?>
