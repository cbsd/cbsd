<?php
/*
	session.inc
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
class Session {
	/**
	 * Initialize default session variables.
	 */
	private static function _init() {
		$_SESSION['login'] = TRUE;
		$_SESSION['authtoken'] = crypt(session_id());
		$_SESSION['ts'] = time();
	}

	/**
	 * Start session.
	 */
	public static function start() {
		session_start();
	}

	/**
	 * Destroy session.
	 */
	public static function destroy() {
		session_start();
		session_destroy();
	}

	/**
	 * Initialize administrator.
	 */
	public static function initAdmin() {
		Session::_init();
		$_SESSION['admin'] = TRUE;
	}

	/**
	 * Initialize user.
	 * @param uid The user ID.
	 * @param uname The user name.
	 */
	public static function initUser($uid, $uname, $admin = FALSE) {
		Session::_init();
		$_SESSION['uid'] = $uid;
		$_SESSION['uname'] = $uname;
		$_SESSION['admin'] = $admin;
	}

	/**
	 * Has the current user administration permissions?
	 * @return TRUE if the current user has administration permissions,
	 * otherwise false.
	 */
	public static function isAdmin() {
		if (!isset($_SESSION['admin']) || !$_SESSION['admin'])
			return FALSE;
		return TRUE;
	}

	/**
	 * Is the login flag set?
	 */
	public static function isLogin() {
		if (!isset($_SESSION['login']) || !$_SESSION['login'])
			return FALSE;
	 	return $_SESSION['login'];
	}

	/**
	 * Validate the given token.
	 * @param authtoken The token to be validated.
	 * @return TRUE if the token is valid, otherwise FALSE.
	 */
	public static function isValidAuthToken($authtoken) {
		if (!isset($_SESSION['authtoken']) || !$_SESSION['authtoken'])
			return FALSE;
		return ($authtoken === $_SESSION['authtoken']);
	}

	/**
	 * Get the current authentication token.
	 * @return The current authentication token.
	 */
	public static function getAuthToken() {
		return $_SESSION['authtoken'];
	}

	/**
	 * Get the current user name.
	 * @return Returns the current user name, otherwise FALSE.
	 */
	public static function getUserName() {
		if (!isset($_SESSION['uname']) || !$_SESSION['uname'])
			return FALSE;
	 	return $_SESSION['uname'];
	}

	/**
	 * Get the current user ID.
	 * @return Returns the current user ID, otherwise FALSE.
	 */
	public static function getUserId() {
		if (!isset($_SESSION['uid']) || !$_SESSION['uid'])
			return FALSE;
	 	return $_SESSION['uid'];
	}
}
?>
