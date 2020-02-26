<?php
header('Content-Type: text/event-stream');
header('Cache-Control: no-cache');
header('Connection: keep-alive');
header('X-Accel-Buffering: no');

set_time_limit(600);
ignore_user_abort(true);

function doEvent($data){
	echo("event: new-msgs\ndata: ".$data."\n\n");
	while (@ob_end_flush());
	flush(); 
}

$CBSD=new Redis();
if(!($CBSD->pconnect("127.0.0.1", 6379))){
	doEvent('{"cmd":"error", "error":"Connecting to Redis has failed!"}');
	exit;
}

if(!($CBSD->auth("password"))){
	doEvent('{"cmd":"error", "error":"Authenticating to Redis has failed!"}');
	exit;
}
$CBSD->setOption (Redis::OPT_READ_TIMEOUT, 600);

doEvent('{"cmd":"initgui"}');

function handle_cbsd_event($redis, $chan, $msg){
//	switch($chan){
//		case "cbsd_events": 
			// Just passtrough for now
			doEvent($msg);
//			break;			


//	}
}

$CBSD->Subscribe(array("cbsd_events"), 'handle_cbsd_event');
