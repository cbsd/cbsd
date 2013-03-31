<?php

// import main lib
require('includes/main.php');

// select page
if (@isset($_GET['executecommand']))
 $content = content_handle('internal', 'executecommand');
else
 redirect_url('status.php');

// serve page
page_handle($content);

?>
