<?php
//this code makesure you are registered and logged in before you can mess with teh system
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: login.php');
   exit;
}
?>

<frameset cols="150,*">
<frame src="side.php" name="side" frameborder="1">
<frame src="title.php" name="main" frameborder="1">
</frameset>
</html>
