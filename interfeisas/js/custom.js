
var folder = "";
var requestCounter = 0;
var stopRequests = false;

$(document).ready(function() {
    $("#degree-slider").slider();
    $("#gif-quality-slider").slider();
    $("#gif-size-slider").slider();
    $("#dry-run-slider").slider();
    
    $("#degree-slider").on("slide", function(slideEvt) {
        if ((360 % slideEvt.value) != 0) {
            $("#degree-slider-val").text("NaN");
            $("#start-btn").prop("disabled", true);
        } else {
            this.turnDegrees = 360/slideEvt.value;
            $("#degree-slider-val").text(this.turnDegrees);
            $("#start-btn").prop("disabled", false);
        }
    });
});

function percentage(num, per) {
  return ((num/per).toFixed(3))*100;
}

function prepSession() {
    var degs = document.getElementById("degree-slider").value;
    if ((360 % degs) == 0) {
        document.getElementById("start-btn").disabled = true;
        document.getElementById("stop-btn").disabled = false;
        document.getElementById("file-format-ext").disabled = true;
        document.getElementById("session-progress-block").style.display = "block";
        document.getElementById("session-error").style.display = "none";
        document.getElementById("session-success").style.display = "none";
        document.getElementById("session-warning").style.display = "none";
        this.stopRequests = false;
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4) {
                if (this.status == 200) {
                    folder = this.responseText;
                    startSession();
                } else {
                    document.getElementById("start-btn").disabled = false;
                    document.getElementById("stop-btn").disabled = true;
                    document.getElementById("session-progress-block").style.display = "none";
                    var errorDiv = document.getElementById("session-error");
                    errorDiv.innerHTML = "Error sending request to server, check connection";
                    errorDiv.style.display = "block";
                }
            }
        };
        xhttp.open("GET", "backend.php?f=createFolder", true);
        xhttp.send();
    }
}

function doRequest(pBar, pLabel, numOfPics, turnDegrees, extension) {
    this.requestCounter++;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            perc = percentage(requestCounter, numOfPics);
            pBar.style.width = perc+"%";
            pBar.setAttribute("aria-valuenow", perc);
            if (!stopRequests && requestCounter < numOfPics) {
                // hopefully we don't hit the recursion limit here lol
                doRequest(pBar, pLabel, numOfPics, turnDegrees, extension);
                pLabel.innerHTML = "Doing photo "+requestCounter+" out of "+numOfPics+":";
            } else {
                if (stopRequests) {
                    var warningDiv = document.getElementById("session-warning");
                    warningDiv.innerHTML = "Session stopped by user";
                    warningDiv.style.display = "block";
                } else {
                    var successDiv = document.getElementById("session-success");
                    successDiv.innerHTML = "Session finished successfully";
                    successDiv.style.display = "block";
                }
                document.getElementById("start-btn").disabled = false;
                document.getElementById("stop-btn").disabled = true;
                document.getElementById("get-files-btn").disabled = false;
                document.getElementById("file-format-ext").disabled = false;
            }
        }
    };
    xhttp.open("GET", "backend.php?f=rotateAndDoPhoto&turnDegrees="+turnDegrees+"&folder="+btoa(folder)+"&ext="+extension, true);
    xhttp.send();
}

function startSession() {
    var numOfPics = document.getElementById("degree-slider-val").innerHTML;
    var pBar = document.getElementById("session-progress-bar");
    var pLabel = document.getElementById("session-progress-label");
    var turnDegrees = document.getElementById("degree-slider").value;
    var extension = document.getElementById("file-format-ext").value.trim();
    pLabel.innerHTML = "Doing photo 1 out of "+numOfPics+":";
    this.requestCounter = 0;
    this.doRequest(pBar, pLabel, numOfPics, turnDegrees, extension);
}

function stopSession() {
    document.getElementById("start-btn").disabled = false;
    document.getElementById("stop-btn").disabled = true;
    document.getElementById("get-files-btn").disabled = false;
    document.getElementById("file-format-ext").disabled = false
    this.stopRequests = true;
}

function showPreview() {
    document.getElementById("preview-btn").disabled = true;
    document.getElementById("ajax-preview-spinner").style.display = "block";
    document.getElementById("preview-error").style.display = "none";
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                document.getElementById("preview-img").style.display = "block";
                document.getElementById("preview-img-link").href = atob(this.responseText);
                document.getElementById("preview-img").src = atob(this.responseText);
            } else {
                var errorDiv = document.getElementById("preview-error");
                errorDiv.innerHTML = "Error sending request to server, check connection";
                errorDiv.style.display = "block";
            }
            document.getElementById("preview-btn").disabled = false;
            document.getElementById("ajax-preview-spinner").style.display = "none";
        }
    };
    xhttp.open("GET", "backend.php?f=showPreview", true);
    xhttp.send();
}

function getLatestFiles() {
    document.getElementById("get-files-btn").disabled = true;
    window.open(
        "backend.php?f=getLatestFiles",
        'popUpWindow',
        'height=300,width=300,left=100,top=100,resizable=yes,scrollbars=no,toolbar=no,menubar=no,location=no,status=yes'
    );
    document.getElementById("get-files-btn").disabled = false;
}

function downloadSession() {
    document.getElementById("download-session-btn").disabled = true;
    var selected = document.getElementById("session-select-download").value;
    window.open(
        "backend.php?f=downloadSession&session="+btoa(selected),
        'popUpWindow',
        'height=300,width=300,left=100,top=100,resizable=yes,scrollbars=no,toolbar=no,menubar=no,location=no,status=yes'
    );
    document.getElementById("download-session-btn").disabled = false;
}

function refreshSessionList() {
    document.getElementById("gif-maker-error").style.display = "none";
    document.getElementById("ajax-refresh-spinner").style.display = "block";
    document.getElementById("refresh-btn").disabled = true;
    document.getElementById("make-gif-btn").disabled = false;
    document.getElementById("preview-session-btn").disabled = false;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                var obj = JSON.parse(this.responseText);
                var selectList = document.getElementById("session-select");
                // remove old entries first
                for (var i = selectList.options.length-1; i >= 0; i--) {
                    selectList.remove(i);
                }
                // now add new entries
                for (var i = 0; i < obj.length; i++) {
                    var opt = document.createElement('option');
                    opt.appendChild(document.createTextNode(obj[i]));
                    opt.value = obj[i];
                    selectList.appendChild(opt);
                }
                document.getElementById("make-gif-btn").disabled = false;
                document.getElementById("preview-session-btn").disabled = false;
            } else {
                var errorDiv = document.getElementById("gif-maker-error");
                errorDiv.innerHTML = "Error sending request to server, check connection";
                errorDiv.style.display = "block";
            }
            document.getElementById("ajax-refresh-spinner").style.display = "none";
            document.getElementById("refresh-btn").disabled = false;
        }
    };
    xhttp.open("GET", "backend.php?f=refreshSessionList", true);
    xhttp.send();
}

function refreshDownloadList() {
    document.getElementById("message-download-alert").style.display = "none";
    document.getElementById("error-download-alert").style.display = "none";
    document.getElementById("ajax-refresh-download-spinner").style.display = "block";
    document.getElementById("refresh-download-btn").disabled = true;
    document.getElementById("download-session-btn").disabled = true;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                var obj = JSON.parse(this.responseText);
                var selectList = document.getElementById("session-select-download");
                // remove old entries first
                for (var i = selectList.options.length-1; i >= 0; i--) {
                    selectList.remove(i);
                }
                // now add new entries
                for (var i = 0; i < obj.length; i++) {
                    var opt = document.createElement('option');
                    opt.appendChild(document.createTextNode(obj[i]));
                    opt.value = obj[i];
                    selectList.appendChild(opt);
                }
                document.getElementById("download-session-btn").disabled = false;
                getSessionInfo();
            } else {
                var errorDiv = document.getElementById("error-download-alert");
                errorDiv.innerHTML = "Error sending request to server, check connection";
                errorDiv.style.display = "block";
            }
            document.getElementById("ajax-refresh-download-spinner").style.display = "none";
            document.getElementById("refresh-download-btn").disabled = false;
        }
    };
    xhttp.open("GET", "backend.php?f=refreshSessionList", true);
    xhttp.send();
}

function getSessionInfo() {
    document.getElementById("download-session-btn").disabled = true;
    document.getElementById("refresh-download-btn").disabled = true;
    document.getElementById("ajax-refresh-download-spinner").style.display = "block";
    var selected = document.getElementById("session-select-download").value;
    if (selected == "") return;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                if (this.responseText != "false") {
                    var sessionMsg = document.getElementById("message-download-alert");
                    var obj = JSON.parse(this.responseText);
                    sessionMsg.innerHTML = "This session contains "+obj["file_count"]+" files";
                    sessionMsg.style.display = "block";
                    document.getElementById("refresh-download-btn").disabled = false;
                    document.getElementById("download-session-btn").disabled = false;
                } else {
                    var errorDiv = document.getElementById("error-download-alert");
                    errorDiv.innerHTML = "Bad response from server, check parameters";
                    errorDiv.style.display = "block";
                }
            } else {
                var errorDiv = document.getElementById("error-download-alert");
                errorDiv.innerHTML = "Error sending request to server, check connection";
                errorDiv.style.display = "block";
            }
            document.getElementById("ajax-refresh-download-spinner").style.display = "none";
        }
    };
    xhttp.open("GET", "backend.php?f=getSessionInfo&session="+btoa(selected), true);
    xhttp.send();
}

function showSessionPreview() {
    var selected = document.getElementById("session-select").value;
    window.open("backend.php?f=showSessionPreview&session="+btoa(selected), "_blank");
}

function waitForImagesLoaded(imageURLs, callback) {
    var imageElements = [];
    var remaining = imageURLs.length;
    var onEachImageLoad = function() {
        if (--remaining === 0 && callback) {
            callback(imageElements);
        }
    };

   for (var i = 0, len = imageURLs.length; i < len; i++) {
        var img = new Image();
        img.onload = onEachImageLoad;
        img.src = imageURLs[i];
        imageElements.push(img);
   }
}

function makeGif(infoArr) {
    var gifQuality = document.getElementById("gif-quality-slider").value*10;
    var gifSize = document.getElementById("gif-size-slider").value;
    
    var d = Math.round((gifSize/100)*infoArr['dimensions'][0]);
    var newWidth = infoArr['dimensions'][0]-d;
    d = Math.round((gifSize/100)*infoArr['dimensions'][1]);
    var newHeight = infoArr['dimensions'][1]-d;
    
    var gif = new GIF({
        workers: 2,
        quality: gifQuality,
        width: newWidth,
        height: newHeight,
        workerScript: "js/gifjs/gif.worker.js",
    });
    
    var pBar = document.getElementById("gif-progress-bar");
    var pLabel = document.getElementById("gif-progress-num");
    
    waitForImagesLoaded(infoArr['images'], function(images) {
        for (var i = 0; i < images.length; i++) {
            gif.addFrame(images[i], {delay: 35});
        }
        gif.render();
    });

    document.getElementById("gif-loading-spinner").style.display = "none";
    
    gif.on('finished', function(blob) {
        var gif = URL.createObjectURL(blob);
        document.getElementById("make-gif-btn").disabled = false;
        document.getElementById("preview-gif-link").href = gif;
        document.getElementById("preview-gif").src = gif;
        document.getElementById("download-gif-btn").href = gif;
        document.getElementById("download-gif-block").style.display = "block";
    });
    
    gif.on('progress', function(progress) {
        var progress = Math.round(progress*100);
        pBar.style.width = progress+"%";
        pBar.setAttribute("aria-valuenow", progress);
        pLabel.innerHTML = progress+"%";
    });
}

function prepGifMaker() {
    var selected = document.getElementById("session-select").value;
    document.getElementById("gif-progress-block").style.display = "block";
    document.getElementById("download-gif-block").style.display = "none";
    document.getElementById("gif-maker-error").style.display = "none";
    document.getElementById("gif-maker-warning").style.display = "none";
    document.getElementById("gif-loading-spinner").style.display = "block";
    document.getElementById("make-gif-btn").disabled = true;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                if (this.responseText == 'false') {
                    var warningDiv = document.getElementById("gif-maker-warning");
                    warningDiv.innerHTML = "This session is empty";
                    warningDiv.style.display = "block";
                    document.getElementById("gif-progress-block").style.display = "none";
                } else {
                    var infoArr = JSON.parse(this.responseText);
                    makeGif(infoArr);
                }
            } else {
                var errorDiv = document.getElementById("gif-maker-error");
                errorDiv.innerHTML = "Error sending request to server, check connection";
                errorDiv.style.display = "block";
            }
            document.getElementById("make-gif-btn").disabled = false;
        }
    };
    xhttp.open("GET", "backend.php?f=getSessionFileList&session="+btoa(selected), true);
    xhttp.send();
}

function drySpinTurntable() {
    document.getElementById("dry-run-btn").disabled = true;
    document.getElementById("ajax-dry-run-spinner").style.display = "block";
    var spinDegs = document.getElementById("dry-run-slider").value;
    var spinReverse = "false";
    if (document.getElementById("dry-run-reverse").checked) {
        spinReverse = "true";
    }
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                document.getElementById("dry-run-btn").disabled = false;
                document.getElementById("ajax-dry-run-spinner").style.display = "none";
            } else {
                var errorDiv = document.getElementById("dry-run-error");
                errorDiv.innerHTML = "Error sending request to server, check connection";
                errorDiv.style.display = "block";
            }
            document.getElementById("dry-run-btn").disabled = false;
            document.getElementById("ajax-dry-run-spinner").style.display = "none";
        }
    };
    xhttp.open("GET", "backend.php?f=drySpinTurntable&deg="+btoa(spinDegs)+"&reverse="+spinReverse, true);
    xhttp.send();
}

function refreshStorageSpace() {
    document.getElementById("refresh-storage-btn").disabled = true;
    document.getElementById("purge-storage-btn").disabled = true;
    document.getElementById("purge-storage-alert").style.display = "none";
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                var obj = JSON.parse(this.responseText);
            } else {
                var obj = ["NaN", "NaN"];
            }
            document.getElementById("total-space").innerHTML = obj["total"];
            document.getElementById("available-space").innerHTML = obj["available"];
            document.getElementById("refresh-storage-btn").disabled = false;
            document.getElementById("purge-storage-btn").disabled = false;
        }
    };
    xhttp.open("GET", "backend.php?f=refreshStorageSpace", true);
    xhttp.send();
}

function purgeStorage() {
    if (!confirm("Are you sure you want to delete everything?")) return;
    document.getElementById("refresh-storage-btn").disabled = true;
    document.getElementById("purge-storage-btn").disabled = true;
    document.getElementById("purge-storage-alert").style.display = "none";
    var txt = "";
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                refreshStorageSpace();
                txt = "Storage has been cleared";
            } else {
                txt = "Something went wrong";
            }
            document.getElementById("refresh-storage-btn").disabled = false;
            document.getElementById("purge-storage-btn").disabled = false;
            document.getElementById("purge-storage-alert").style.display = "block";
            document.getElementById("purge-storage-alert").innerHTML = txt;
        }
    };
    xhttp.open("GET", "backend.php?f=purgeStorage", true);
    xhttp.send();
}

function refreshCameraInfo() {
    document.getElementById("refresh-camera-info-btn").disabled = true;
    var txt = "";
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                txt = atob(this.responseText);
            } else {
                txt = "Something went wrong";
            }
            document.getElementById("refresh-camera-info-btn").disabled = false;
            document.getElementById("camera-info-textarea").innerHTML = txt;
        }
    };
    xhttp.open("GET", "backend.php?f=refreshCameraInfo", true);
    xhttp.send();
}
