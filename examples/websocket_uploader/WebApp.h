/*
Copyright (c) 2024 Adam Kaniewski

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <string>

namespace WebApp {
const static std::string INDEX_HTML = R"""(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  #uploader_box {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    width: 30%;
    height: 30%;
  }
</style>
<title>WebSocket Uploader Example</title>
<script src="uploader.js"></script>
<script>
  function init() {
    var currentUrl = new URL(window.location.href);
    var wsUrl = "ws://" + currentUrl.host;
    var uploader = new Uploader(wsUrl, "./", document.getElementById("uploader_box"));
    document.uploader = uploader;
  }
</script>
</head>
<body onload="init()">
<div id="uploader_box"></div>
</body>
</html>
)""";

const static std::string UPLOADER_JS = R"""(
class Uploader {
  constructor(wsUrl, targetDir, parentObj) {
    this.wsUrl = wsUrl;
    this.targetDir = targetDir;
    this.parentObj = parentObj;
    this.fileOffset = 0;
    this.currentFileIndex = 0;
    this.chunkSize = 1024*512;
    this.defaultText = "Drag & Drop<br>or<br>Click";
    this.files = null;
    this.websocket = null;
    this.dropZone = null;
    this.fileInput = null;
    this.textParagraph = null;
    this.createNode();
  }

  resetFilesStatus() {
    this.files = null;
    this.fileOffset = 0;
    this.currentFileIndex = 0;
  }

  createWS() {
    let self = this;
    this.websocket  = new WebSocket(this.wsUrl);
    this.websocket.addEventListener("open", (ev) => {
      this.sendUploadRequest();
    });
    this.websocket.addEventListener("message", (ev) => {
      this.onWsMessage(ev);
    });
    this.websocket.addEventListener("close", (ev) => {
      this.onWsClose();
    });
    this.websocket.addEventListener("error", (ev) => {
      this.onWsError();
    });
  }

  createNode() {
    this.fileInput = document.createElement("input");
    this.fileInput.type = "file";
    this.fileInput.multiple="multiple";
    this.fileInput.style = "display: none;"
    this.fileInput.addEventListener('change', (e) => {
      var files = e.target.files;
      this.handleFiles(files);
    });

    this.dropZone = document.createElement("div");
    this.dropZone.innerHTML = this.defaultText;
    this.dropZone.style="\
      width: 100%; \
      height: 100%;\
      border: 2px dashed #aaa;\
      border-radius: 10px;\
      text-align: center;\
      line-height: 5vb; \
      font-family: Arial, sans-serif;\
      display: flex; \
      justify-content: center; \
      align-items: center; \
      cursor: pointer;";

    this.dropZone.addEventListener('dragover', (e) => {
      e.preventDefault();
      e.stopPropagation();
      this.dropZone.style.backgroundColor = "#f0f0f0";
    });

    this.dropZone.addEventListener('dragleave', (e) => {
      e.preventDefault();
      e.stopPropagation();
      this.dropZone.style.backgroundColor = "white";
    });

    this.dropZone.addEventListener('drop', (e) => {
      e.preventDefault();
      e.stopPropagation();
      this.dropZone.style.backgroundColor = "white";
      var files = e.dataTransfer.files;
      this.handleFiles(files);
    });

    this.dropZone.addEventListener('click', (e) => {
      this.fileInput.click();
    });

    this.dropZone.appendChild(this.fileInput);
    this.parentObj.appendChild(this.dropZone);
  }

  handleFiles(files) {
    if (files.length > 0) {
      this.files = files;
      this.createWS();
    } else {
      this.setText("Error : empty file list");
      this.resetFilesStatus();
    }
  }

  setText(msg) {
    var txt = this.defaultText;
    if(msg != null) {
      txt = msg;
    }
    this.dropZone.innerHTML = txt;
  }

  onWsOpen() {
    this.sendUploadRequest();
  }

  onWsMessage(msg) {
    var json = JSON.parse(msg.data);
    if(json.type === "upload_resp") {
      if(json.err === 0) {
        this.sendDataChunk();
      } else {
        this.setText("Upload rejected : " + json.err_msg);
        this.onWsError(null);
      }
    } else {
      this.setText("Unknow server response : " + json.type);
      this.onWsError(null);
    }
  }

  onWsClose() {
    this.websocket = null;
    this.resetFilesStatus();
  }

  onWsError(err) {
    if(this.websocket !== null && this.websocket !== undefined) {
      this.websocket.close();
    }
    this.resetFilesStatus();
  }

  sendUploadRequest() {
    var file = this.files[this.currentFileIndex];
    var request = {
      type: "upload_req",
      file_name: file.name,
      file_directory_path: this.targetDir,
      expected_chunk_size: this.chunkSize,
      expected_file_size: file.size
    };

    this.sendWsMessage(JSON.stringify(request));
  }

  sendWsMessage(msg) {
    if(this.websocket != null && this.websocket.readyState == WebSocket.OPEN) {
      this.websocket.send(msg);
    }
  }

  onTrnasferComplete() {
    this.setText("Transfer Completed");
    this.fileOffset = 0;
    if(this.files.length - 1 > this.currentFileIndex) {
      this.currentFileIndex += 1;
      this.sendUploadRequest();
    } else {
      this.resetFilesStatus();
    }
  }

  sendDataChunk(){
    var file = this.files[this.currentFileIndex];
    if(this.fileOffset < file.size) {
      var slice = this.chunkSize;
      if(slice > file.size - this.fileOffset) {
        slice = file.size - this.fileOffset;
      }
      var chunk = file.slice(this.fileOffset, this.fileOffset + slice);
      this.fileOffset += slice;
      this.setText("File: " + file.name + " [ " +  this.fileOffset + " / " + file.size + " ]");
      var reader = new FileReader();
      reader.addEventListener("load", (evt) => {
        this.sendWsMessage(evt.target.result);
      });
      reader.readAsArrayBuffer(chunk);
    } else {
      this.onTrnasferComplete();
    }
  }
};
)""";
}; //namespace WebApp
