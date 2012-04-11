<?php
session_start();

// if the user is logged in, unset the session
if (isset($_SESSION['db_admin_is_logged_in'])) {
   unset($_SESSION['db_admin_is_logged_in']);
}

// now that the user is logged out,
// go to login page
header('Location: login.php');
?>

