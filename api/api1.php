<?php

include_once 'config.php';


die(json_encode(executeCommand($_GET['c'])));


function executeCommand($cmd) {
	switch ($cmd){
		case 'getId':		return getId();
		case 'getName':		return getName();
		case 'getStatus':	return getStatus();
		case 'getRotate':	return getRotate();
		case 'getPosition': return getPosition();
		case 'getDelay':	return getDelay();

		case 'getLed':		return getLed($_GET['p1']);
		case 'getButton':	return getButton($_GET['p1']);
		case 'getRelay':	return getRelay($_GET['p1']);

		case 'setDelay':	return setDelay($_GET['p1']);
		case 'setRotate':	return setRotate($_GET['p1']);
		case 'setLed':		return setLed($_GET['p1'], $_GET['p2']);
		case 'setRelay':	return setRelay($_GET['p1'], $_GET['p2']);
		default: return false;
	}
}


function getId() {
	return 2;
}
function getName() {
	return $cfg['device']['name'];
}
function getStatus() {
	return str2arr(useCommand('status'));
}
function getRotate() {
	$tmp = str2arr(useCommand('status'));
	return (is_array($tmp))?$tmp['target']:false;
}
function getPosition() {
	$tmp = str2arr(useCommand('status'));
	if (is_array($tmp)) {
		$res = $tmp['position'] * -1;
		$res = round($res/360*400);
		if ( abs($res) == 400 || $res == 0 ) {
			switch (rand(1,3)) {
				case 1;	$res = 0; break;
				case 2: $res = 400; break;
				case 3: $res = -400; break;
			}
		}
		return (string)$res;
	}
	return false;
}
function getDelay() {
	$tmp = str2arr(useCommand('status'));
	return (is_array($tmp))?$tmp['sleep']:false;
}


function getLed($led) {
	$tmp = array('led1'=>'1','led2'=>'1');
	if (is_array($tmp) && isset($tmp["led{$led}"])) {
		return $tmp["led{$led}"];
	}
	return false;
}
function getButton($btn) {
	$tmp = array('btn1'=>'1','btn2'=>'0');
	if (is_array($tmp) && isset($tmp["btn{$btn}"])) {
		return $tmp["btn{$btn}"];
	}
	return false;
}
function getRelay($rel) {
	$tmp = array('rel1'=>'1','rel2'=>'1');
	if (is_array($tmp) && isset($tmp["rel{$rel}"])) {
		return $tmp["rel{$rel}"];
	}
	return false;
}


function setDelay($del) {
	if ($del > 0 && $del <= 10 ) {
		useCommand(sprintf('stop'));
	}
	return true;
}
function setRotate($rot) {
	// sukeklis2 positive steps are CW and not
	// CCW as in sukeklis1, so reverse direction.
	$rot *= -1;
	$rot = $rot/400*360; 
	useCommand(sprintf('target %.5f',$rot));
	return true;
}
function setLed($led,$on) {
	// does nothing on sukeklis 2.0
	return false;
}
function setRelay($rel,$on) {
	// does nothing on sukeklis 2.0
	return false;
}


/* ========================================================================== */


function useCommand($cmd) { global $cfg;

	// lock device ===
	$fp = fopen("/tmp/php_{$cfg['device']['name']}.lock", "w+");if (flock($fp, LOCK_EX)) {

	if ( preg_match("/^status/Umis", $cmd) ) {
		$result = file($cfg['device']['stat']);
	} else {
		file_put_contents($cfg['device']['cmd'], $cmd);
		$result = '';
	}

	// unlock device ===
	flock($fp, LOCK_UN); } else { return false; } fclose($fp);

	return $result;
}

function str2arr($str) {
	$result = false;
	if (is_array($str)) {
		foreach ($str as $field) {
			list($k,$v) = explode("=",$field);
			if (!empty($v)) 
				$result[trim($k)] = trim($v);
		}
	}
	return $result;
}
