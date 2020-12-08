#!/usr/bin/php
<?php
define("RPI_REV",1);

define("PIN_EN",    23 );
define("PIN_DIR",   4  );
define("PIN_STEP",  18 );
define("PIN_MS1",   25 );
define("PIN_MS2",   24 );
define("PIN_MS3",   22 );
define("PIN_TAMP1", 17 );
if (RPI_REV == 1) define("PIN_TAMP2", 21 );
else define("PIN_TAMP2", 27 );

define("DIR_CW",  0);
define("DIR_CCW", 1);
define("RES_1",   0);
define("RES_2",   1);
define("RES_4",   2);
define("RES_8",   3);
define("RES_16",  7);

define("STEP_DEL_MIN",  1000);
define("STEP_DEL_MAX", 50000);
define("STEP_ACC",     1.1);

$pins = array(PIN_EN, PIN_DIR, PIN_STEP, PIN_MS1, PIN_MS2, PIN_MS3, PIN_TAMP1, PIN_TAMP2);
$status = array();
$pos=0;

init($pins);
config(DIR_CW,RES_1);
step_to(1200);



function init($pins) {
	foreach ($pins as $pin) {
		shell_exec("echo {$pin} > /sys/class/gpio/export");
		shell_exec("echo out > /sys/class/gpio/gpio{$pin}/direction");
		set($pin,0);
	}
}
function config($dir,$res) {
	set_dir($dir);
	set_res($res);
	set(PIN_EN, 1);
}
function set($pin,$val) { global $status;
	$pin = (int)$pin;
	$val = (int)(bool)$val;
	$status[$pin] = $val;
	shell_exec("echo {$val} > /sys/class/gpio/gpio{$pin}/value");
}
function set_dir($dir) {
	set(PIN_DIR, $dir);
}
function set_res($res) {
	set(PIN_MS1, ($res>>2)&1);
	set(PIN_MS2, ($res>>1)&1);
	set(PIN_MS3, $res&1);
}
function step($dir=null) { global $status, $pos;
	if ($dir!==null) set_dir($dir);
	$pin = PIN_STEP;
	shell_exec("echo 1 > /sys/class/gpio/gpio{$pin}/value ; echo 0 > /sys/class/gpio/gpio{$pin}/value");
	//set(PIN_STEP, 1);
	//usleep(1);
	//set(PIN_STEP, 0);
	//usleep(1);
	$pos += ($status[PIN_DIR]==DIR_CW)?1:-1;
}
function step_to($target) { global $status, $pos;
	$del = STEP_DEL_MAX;
	$speed_steps = 0;
	if ($pos<$target) {
		set_dir(DIR_CW);
	} else {
		set_dir(DIR_CCW);
	}
	for ($start=$pos; $pos!=$target;) {
		step();
		$first = abs(abs($pos)-abs($start));
		$second = abs(abs($pos)-abs($target));
		if ( $first < $second ) {
printf("--> now at %d going to %d with %f dellay\n",$pos,$target,$del);
			// speed up
			if (STEP_DEL_MIN < $del) {
				$del /= STEP_ACC;
				$speed_steps++;
			}
		} else {
printf("<-- now at %d going to %d with %f dellay\n",$pos,$target,$del);
			// slow down
			if (STEP_DEL_MAX > $del && $speed_steps > $second) {
				$del *= STEP_ACC;
			}
		}
		usleep($del);
	}
}
