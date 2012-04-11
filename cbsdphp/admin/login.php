<?php
// we must never forget to start the session
session_start();

$errorMessage = '';
if (isset($_POST['uid']) && isset($_POST['upass'])) {
   include '../includes/config.php';

$conn = mysql_connect($MySQL_Host, $MySQL_User, $MySQL_Passw) or die
('Error connecting to mysql');
mysql_select_db($db);

   $userId = $_POST['uid'];
   $pwd = $_POST['upass'];
   $password = md5(sha1(md5(base64_encode($pwd))));

   $sql = "SELECT user_id
           FROM Users
           WHERE user_id = '$userId'
           AND user_password = '$password'
           AND user_lvl = 'admin'" ;

   $result = mysql_query($sql)
             or die('Query failed. ' . mysql_error());

   if (mysql_num_rows($result) == 1) {
      // the user id and password match,
      // set the session
      $_SESSION['db_admin_is_logged_in'] = true;

      // after login we move to the main page
   echo '<META HTTP-EQUIV="Refresh" Content="1; URL=main.php">';
   echo 'Authentication successful!';
   echo '</br>';
   echo 'Logging in now';
      exit;
   } else {
      $errorMessage = 'You were unable to log in. This could be due to many reasons, contact your system admin for help';
echo $errorMessage;
}
   include '../includes/closedb.php';
}
?>
   
<html>
<head>
<title>Login</title>
</head>   
<body>
<form method="post">
Username:<input type="text" size="12" maxlength="16" name="uid"><br />
Password:<input type="password" size="12" maxlength="64" name="upass"><br />
<input type="submit" value="submit" name="submit"><br />
</form><br />

