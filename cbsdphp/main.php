<?php
//this code makesure you are registered and logged in before you can mess with teh system
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_is_logged_in'])
   || $_SESSION['db_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: login.php');
   exit;
}
?>

FOSVM test page... Will be working sooner or later!
<br/>
<a href="logout.php">Logout</a>
