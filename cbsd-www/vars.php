<?php

$buf=print_r($_SERVER,true);

if($_SERVER['REQUEST_METHOD']=='POST')
{
	$buf.=PHP_EOL.'---- POSTS START ----'.PHP_EOL;
	$buf.=print_r($_POST,true);
	$buf.='---- POSTS END ----'.PHP_EOL.PHP_EOL;
}

$buf.=PHP_EOL.'---- GETS START ----'.PHP_EOL;
$buf.=print_r($_GET,true);
$buf.='---- GETS END ----'.PHP_EOL.PHP_EOL;

$buf.=PHP_EOL.'---- COOKIES START ----'.PHP_EOL;
$buf.=print_r($_COOKIE,true);
$buf.='---- COOKIES END ----'.PHP_EOL.PHP_EOL;

$buf.=print_r($_ENV,true);

//$buf.='User IP: '.$_SERVER['REMOTE_ADDR'].PHP_EOL;

echo '<pre>',$buf;