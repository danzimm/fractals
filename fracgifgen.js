#!/usr/bin/env node

var util          = require('util'),
    cp            = require('child_process'),
    fs            = require('fs'),
    i;

var outdir = "fracgif",
    outfilebase = "frac",
    color = "B562E0",
    delay = "10",
    initial = 20,
    end = 200;

var params = [];
for (i = initial; i <= end; i++) {
  params.push([i / 10, i]);
}

try {
  fs.mkdirSync(outdir);
} catch(e) {}

function createGif() {
  setTimeout(function() {
    var filelist = "", j;
    for (j = initial; j <= end; j++) {
      filelist += outdir + "/" + outfilebase + j + ".png ";
    }
    cp.exec("convert -delay " + delay + " -loop 0 " + filelist + outdir + "/" + outfilebase + ".gif", function(error, stdout, stderr) {
      if (error) {
        console.log("Error creating gif: " + error);
      }
    });
  }, 1000);
}

i = 0;

params.forEach(function(param, i) {
  if (!fs.existsSync(outdir + "/" + outfilebase + param[1] + ".png")) {
    var command = "./fractalgen -c " + color + " -p " + param[0] + " -o " + outdir + "/" + outfilebase + param[1] + ".png";
    console.log(command);
    cp.exec(command, function(error, stdout, stderr) {
      i++;
      if (error) {
        console.log("Error: " + error + " with " + param + " : " + (stdout + '').replace("\n", "") + " : " + (stderr + '').replace("\n", ""));
      } else {
        console.log("Finished " + param + " : " + (stdout + '').replace("\n", "") + " : " + (stderr + '').replace("\n", ""));
      }
      if (i === end - initial + 1) {
        createGif();
      }
    });
  } else {
    i++;
    if (i === (end - initial + 1)) {
      createGif();
    }
  }
});

