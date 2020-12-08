<?php

include_once 'config.php';


die(json_encode(executeCommand($_GET['c'])));


function executeCommand($cmd) {
	switch ($cmd){
		case 'getStatus':   return getStatus( (!empty($_GET['p1']))?$_GET['p1']:null );
		case 'getTarget':   return getTarget();
		case 'getPosition': return getPosition();

		case 'setTarget':   return setTarget($_GET['p1']);
		case 'setRelative': return setRelative($_GET['p1']);

		case 'setPause':    return useCommand('pause');
		case 'setResume':   return useCommand('resume');
		case 'setStop':     return useCommand('stop');
		case 'setSpin':     return setSpin($_GET['p1']);

		case 'setAcc':      return setAcc($_GET['p1']);
		case 'setMinSleep': return setMinSleep($_GET['p1']);
		case 'setMaxSleep': return setMaxSleep($_GET['p1']);
		case 'setRes':      return seRes($_GET['p1']);

		default: return false;
	}
}


function getStatus($var = null) {
	$res = str2arr(useCommand('status'));
	if ( empty($var) ) {
		return $res;
	} else {
		$res = (isset($res[$var]))?$res[$var]:false;
		if ($res === false) return $res;
		switch ($var) {
			case 'target':
			case 'position':
			case 'acc':
			case 'deg_per_step':
			case 'tolerance':
				$res = (double)$res;
				break;
			case 'spin_dir':
			case 'dir':
			case 'sleep':
			case 'speed':
			case 'min_sleep':
			case 'max_sleep':
			case 'res':
			case 'ms1':
			case 'ms2':
			case 'ms3':
			case 'steps_per_rot':
				$res = (int)$res;
				break;
			case 'enabled':
			case 'pause':
			case 'stop':
			case 'spin':
				$res = (bool)$res;
				break;
		}
		return $res;
	}
}
function getTarget() {
	return getStatus('target');
}
function getPosition() {
	return getStatus('position');
}



function setTarget($pos) {
	useCommand(sprintf('target %.5f',$pos));
	return true;
}
function setRelative($pos) {
	useCommand(sprintf('relative %.5f',$pos));
	return true;
}
function setAcc($f) {
	useCommand(sprintf('acc %.5f',$f));
	return true;
}
function setMinSleep($f) {
	useCommand(sprintf('min_sleep %.5f',$f));
	return true;
}
function setMaxSleep($f) {
	useCommand(sprintf('max_sleep %.5f',$f));
	return true;
}

function setRes($i) {
	useCommand(sprintf('res %d',$i));
	return true;
}
function setSpin($i) {
	useCommand(sprintf('spin %d',$i));
	return true;
}

/* ========================================================================== */


function useCommand($cmd) { global $cfg;

	// lock device ===
	$fp = fopen("/tmp/php_{$cfg['device']['name']}.lock", "w+");if (flock($fp, LOCK_EX)) {

	if ( preg_match("/^status/Umis", $cmd) ) {
		$result = file($cfg['device']['stat']);
	} else {
		file_put_contents($cfg['device']['cmd'], $cmd);
		$result = true;
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
