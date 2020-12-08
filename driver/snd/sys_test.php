#!/usr/bin/php
<?php
openlog("sys_test", LOG_PID | LOG_PERROR, LOG_LOCAL0);

$sleep_time = 3;

sleep($sleep_time);
$mem = getMemArray();

sleep($sleep_time);
$cpu = getCpuLoad();



// check ram and play sound if needed
sleep($sleep_time);
if ( ($mem['Buffers'] + $mem['Cached']) < 10000 && $mem['MemFree'] < 10000 ) {
	logmsg("Low on free RAM!");
	playSound('ram1');
}



// check swap
sleep($sleep_time);
if ( ($mem['SwapTotal'] - $mem['SwapFree']) > 1000  ) {
	logmsg("Using SWAP!");
	playSound('swap1');
}


// check cpu
sleep($sleep_time);
if ($cpu > 2) {
	logmsg("CPU load is hi!");
	playSound('cpu1');
}



function getMemArray() {
	$result = array();
	$tmp = file('/proc/meminfo');
	foreach ($tmp as $line) {
		list($k,$v) = explode(":",$line);
		$result[trim($k)] = (int)trim($v);
	}
	return $result;
}
function getCpuLoad() {
	return shell_exec("/usr/bin/uptime | /usr/bin/awk '{print $(NF-2)*1 }' 2>/dev/null");
}
function playSound($name) {
	shell_exec("/usr/bin/aplay /home/pi/driver/snd/{$name}.wav 2>/dev/null");
}
function logmsg($msg) {
	syslog(LOG_WARNING,$msg);
}

closelog();
