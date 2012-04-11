<?php
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

      // not logged in, move to login page
     header('Location: /admin/login.php');
    exit;
    }
?>
No index set
