<?php

//=========================================
/*
    CRUCIAL INFO:
        if gphoto2 doesn't work, then do chmod +s /usr/bin/gphoto2
        gphoto2 permissions should be as follows:
        -rwsr-sr-x 1 root root
*/
//=========================================
const PHOTO_ROOT = 'photos';
const PREVIEW_TEMPLATE = 'template.html';
const GPHOTO = '/usr/bin/gphoto2';
const GPHOTO_CMD = GPHOTO.' --quiet --capture-image-and-download --filename';
const SUKEKLIS_IP = '10.87.194.10';
const SUKEKLIS_API = 'api2.php';
const SUKEKLIS_CMDS = array(
    'spin' => 'setRelative',
    'stop' => 'setStop',
    'status' => 'getStatus',
    'target' => 'getTarget',
    'position' => 'getPosition',
);
//---------------
$publicFunctions = array(
    'rotateAndDoPhoto',
    'createFolder',
    'showPreview',
    'getLatestFiles',
    'downloadSession',
    'refreshSessionList',
    'showSessionPreview',
    'getSessionFileList',
    'getSessionInfo',
    'drySpinTurntable',
    'refreshStorageSpace',
    'purgeStorage',
    'refreshCameraInfo'
);
//---------------
// execution handler
if (isset($_GET['f']) && in_array($_GET['f'], $publicFunctions)) {
    $_GET['f']();
} else {
    die();
}
//=========================================
function getDirContents($dir, $filesOnly = false, $jpgOnly = false, $flags = SCANDIR_SORT_ASCENDING) {
    if ($filesOnly) {
        $fileArr = scandir($dir, $flags);
        $imgArr = array();
        foreach ($fileArr as $f) {
            if (!is_dir($f)) {
                if ($jpgOnly && (strpos($f, '.jpg') !== false)) {
                    $imgArr[] = $f;
                } else {
                    $imgArr[] = $f;
                }
            }
        }
        return $imgArr;
    } else {
        $folders = array();
        $c = array_diff(scandir($dir, $flags), array('..', '.'));
        foreach ($c as $item) {
            if (is_dir($dir.'/'.$item)) $folders[] = $item;
        }
        return $folders;
    }
}
//=========================================
function rotateAndDoPhoto() {
    if (isset($_GET['turnDegrees']) && is_numeric($_GET['turnDegrees']) && isset($_GET['folder']) && isset($_GET['ext'])) {
        $folder = base64_decode($_GET['folder']);
        $filename = sprintf('%s.%s', time(), $_GET['ext']);
        exec(sprintf('cd "%s" && %s "%s"', $folder, GPHOTO_CMD, $filename));
        $turnDegrees = intval($_GET['turnDegrees']);
        if ($turnDegrees <= 0) {
            echo 'false';
            return;
        }
        file_get_contents(sprintf('http://%s/%s?c=%s&p1=%d', SUKEKLIS_IP, SUKEKLIS_API, SUKEKLIS_CMDS['spin'], $turnDegrees));
        sleep(1);
        $target = intval(file_get_contents(sprintf('http://%s/%s?c=%s', SUKEKLIS_IP, SUKEKLIS_API, SUKEKLIS_CMDS['target'])));
        if ($target == 360) $target = 0;
        $position = 0;
        do {
            sleep(1);
            $position = floatval(file_get_contents(sprintf('http://%s/%s?c=%s', SUKEKLIS_IP, SUKEKLIS_API, SUKEKLIS_CMDS['position'])));
            $position = round($position*50)/50;
            if ($position == 360) $position = 0;
        } while ($position != $target);
        sleep(1);
    }
}
//---------------
function createFolder() {
    $fName = PHOTO_ROOT.'/'.strftime('%Y-%m-%d_%H-%M-%S');
    if (mkdir($fName)) {
        echo $fName;
    } else {
        echo 'false';
    }
}
//=========================================
function showPreview() {
    $filename = sprintf('pre-%s.jpg', time());
    shell_exec(sprintf('cd %s && %s "%s"', PHOTO_ROOT, GPHOTO_CMD, $filename));
    $img = file_get_contents(PHOTO_ROOT.'/'.$filename);
    echo base64_encode($img);
}
//=========================================
function createZipFile($dirName, $prefix = '') {
    $zip = new ZipArchive();
    $tmpFile = tempnam('tmp', '');
    $zip->open($tmpFile, ZipArchive::CREATE);
    $dir = opendir($dirName);
    while ($file = readdir($dir)) {
        if (is_file($dirName.'/'.$file)) {
            $zip->addFile($dirName.'/'.$file, $prefix.'/'.$file);
        }
    }
    $zip->close();
    return $tmpFile;
}
//---------------
function getLatestFiles() {
    $dirName = scandir(PHOTO_ROOT, SCANDIR_SORT_DESCENDING)[0];
    $fullDirName = PHOTO_ROOT.'/'.$dirName;
    $zipFile = createZipFile($fullDirName, $dirName);
    header('Content-disposition: attachment; filename="sukeklis-latest.zip"');
    header('Content-type: application/zip');
    readfile($zipFile);
    unlink($zipFile);
}
//---------------
function downloadSession() {
    if (isset($_GET['session']) && (trim($_GET['session'] != ''))) {
        $session = base64_decode($_GET['session']);
        $zipFile = createZipFile(PHOTO_ROOT.'/'.$session, $session);
        header('Content-disposition: attachment; filename="sukeklis-'.$session.'.zip"');
        header('Content-type: application/zip');
        readfile($zipFile);
        unlink($zipFile);
    }
}
//=========================================
function refreshSessionList() {
    echo json_encode(getDirContents(PHOTO_ROOT, false, false, SCANDIR_SORT_DESCENDING));
}
//=========================================
function showSessionPreview() {
    if (isset($_GET['session']) && (trim($_GET['session'] != ''))) {
        $session = base64_decode($_GET['session']);
        $sessionDir = PHOTO_ROOT.'/'.$session;
        $imgArr = getDirContents($sessionDir, true, true);
        if (count($imgArr) > 0) {
            $infoText = sprintf('Previewing session "%1$s", found %2$d images', $session, count($imgArr));
            $imgHtmlArr = array();
            foreach ($imgArr as $img) {
                if (stripos($img, 'jpg') !== FALSE) {
                    $imgHtmlArr[] = sprintf('<a href="%1$s" data-fancybox="gallery"><img src="%1$s" style="width: 30em" class="img-thumbnail m-1"></a>',$sessionDir.'/'.$img);
                }
            }
            if (count($imgHtmlArr) > 0) {
                $images = implode($imgHtmlArr);
            } else {
                $infoText = 'No JPG files found in this session';
                $images = '';
            }
        } else {
            $infoText = 'No JPG files found in this session';
            $images = '';
        }
    } else {
        $infoText = 'No session was selected';
        $images = '';
    }
    $template = file_get_contents(PREVIEW_TEMPLATE);
    $template = sprintf($template, $infoText, $images);
    echo $template;
}
//=========================================
function getSessionFileList() {
    if (isset($_GET['session']) && (trim($_GET['session'] != ''))) {
        $session = base64_decode($_GET['session']);
        $sessionDir = PHOTO_ROOT.'/'.$session;
        $sessionArr = getDirContents($sessionDir, true, true);
        if (count($sessionArr) > 0) {
            $infoArr = array('dimensions' => array(), 'images' => array());
            $imgInfo = getimagesize($sessionDir.'/'.$sessionArr[0]);
            $infoArr['dimensions'][] = $imgInfo[0];
            $infoArr['dimensions'][] = $imgInfo[1];
            foreach ($sessionArr as $entry) {
                $infoArr['images'][] = $sessionDir.'/'.$entry;
            }
            echo json_encode($infoArr);
        } else {
            echo 'false';
        }
    }
}
//=========================================
function getSessionInfo() {
    if (isset($_GET['session']) && (trim($_GET['session'] != ''))) {
        $session = base64_decode($_GET['session']);
        echo json_encode(array(
            'file_count' => count(getDirContents(PHOTO_ROOT.'/'.$session, true)),
        ));
    } else {
        echo 'false';
    }
}
//=========================================
function drySpinTurntable() {
    if (isset($_GET['deg']) && (trim($_GET['deg'] != ''))) {
        $degs = intval(base64_decode($_GET['deg']));
        $spinInfo = array();
        if ($degs == 45) {
            $spinInfo = array(1, 45);
        } else if ($degs >= 90) {
            $spinInfo = array($degs/90, 90);
        }
        file_get_contents(sprintf('http://%s/%s?c=%s', SUKEKLIS_IP, SUKEKLIS_API, SUKEKLIS_CMDS['stop']));
        sleep(1);
        while ($spinInfo[0] != 0) {
            file_get_contents(sprintf('http://%s/%s?c=%s&p1=%d', SUKEKLIS_IP, SUKEKLIS_API, SUKEKLIS_CMDS['spin'], $spinInfo[1]));
            $spinInfo[0]--;
            sleep(1);
            $target = intval(file_get_contents(sprintf('http://%s/%s?c=%s', SUKEKLIS_IP, SUKEKLIS_API, SUKEKLIS_CMDS['target'])));
            $position = 0;
            do {
                sleep(1);
                $position = intval(file_get_contents(sprintf('http://%s/%s?c=%s', SUKEKLIS_IP, SUKEKLIS_API, SUKEKLIS_CMDS['position'])));
            } while ($position != $target);
            
        }
        sleep(1);
        echo 'true';
    }
}
//=========================================
function refreshStorageSpace() {
    $symbols = array('B', 'KB', 'MB', 'GB');
    $bytes = disk_total_space(PHOTO_ROOT);
    $exp = floor(log($bytes)/log(1024));
    $total = sprintf('%d '.$symbols[$exp], ($bytes/pow(1024, floor($exp))));
    $bytes = disk_free_space(PHOTO_ROOT);
    $exp = floor(log($bytes)/log(1024));
    $available = sprintf('%d '.$symbols[$exp], ($bytes/pow(1024, floor($exp))));
    echo json_encode(array('total' => $total, 'available' => $available));
}
//---------------
function purgeStorage($dir = null) {
    if (!$dir) $dir = PHOTO_ROOT;
    $objects = scandir($dir);
    foreach ($objects as $object) {
        if ($object != "." && $object != "..") {
            if (filetype($dir.'/'.$object) == 'dir') purgeStorage($dir.'/'.$object); else unlink($dir.'/'.$object);
        }
    }
    reset($objects);
    if ($dir != PHOTO_ROOT) rmdir($dir);
}
//=========================================
function refreshCameraInfo() {
    $rtn = array();
    exec(sprintf('%s --quiet --auto-detect', GPHOTO), $rtn);
    echo base64_encode(implode("\n", $rtn));
}
//=========================================
?>