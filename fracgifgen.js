#!/usr/bin/env node

var util      = require('util'),
    cp        = require('child_process'),
    fs        = require('fs'),
    optparse  = require('optparse'),
    i,
    switches = [
      ["-o", "--out DIR", "The directory to place all the images created"],
      ["-p", "--prefix PREFIX", "The name for the files to be created"],
      ["-c", "--color HEX", "The hex color for the images to be created"],
      ["-d", "--delay NUMBER", "The delay for between each image in the resulting gif"],
      ["-i", "--initial NUMBER", "The initial parameter to create the gif"],
      ["-f", "--final NUMBER", "The final parameter to create the gif"],
      ["-h", "--help", "Help"],
      ["-g", "--generator something.fg", "The generator used to render the fractals"],
      ["-m", "--multiplier NUMBER", "The number to multiply each value in [initial final] by to be the parameter"],
      ["-n", "--nproc NUMBER", "The max number of processes running at once, defaults to 200"],
      ["-w", "--width NUMBER", "The width"],
      ["--height NUMBER", "The height"]
    ],
    parser = new optparse.OptionParser(switches);

var outdir = "fracgif",
    outfilebase = "frac",
    color = "B562E0",
    delay = "10",
    initial = 20,
    end = 200,
    generator = null,
    multiplier = 0.1,
    nproc = 200,
    width = 1000,
    height = 1000;

parser.on("out", function(d, dir) {
  outdir = dir;
});
parser.on("prefix", function(d, pre) {
  outfilebase = pre;
});
parser.on("color", function(d, col) {
  color = col;
});
parser.on("delay", function(d, del) {
  delay = del;
});
parser.on("initial", function(d, num) {
  initial = num;
});
parser.on("final", function(d, fin) {
  end = fin;
});
parser.on("generator", function(d, gen) {
  generator = gen;
});
parser.on("multiplier", function(d, multi) {
  multiplier = multi;
});
parser.on("nproc", function(d, np) {
  nproc = np;
});
parser.on("width", function(d, wi) {
  width = wi;
});
parser.on("height", function(d, he) {
  height = he;
});
parser.on("help", function() {
  console.log("This utility iterates through parameters from `initial` to `final` (inclusive on both) to create a series of pngs from `fractalgen`. It then creates a gif out of these png.");
  console.log(parser.toString());
  process.exit(0);
});

parser.parse(process.argv);

var params = [], i, queuedParameters = [], running;
for (i = initial; i <= end; i++) {
  params.push([i * multiplier, i]);
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
  }, 2500);
}

i = running = 0;

function enqueueParameter(param) {
  if (running < nproc) {
    var command = "./fractalgen -w " + width + " -h " + height + " -c " + color + " -p " + param[0] + " -o " + outdir + "/" + outfilebase + param[1] + ".png" + (generator ? " -g " + generator : "");
    running++;
    console.log(command);
    cp.exec(command, function(error, stdout, stderr) {
      running--;
      i++;
      if (error) {
        console.log("Error: " + error + " with " + param + " : " + (stdout + '').replace("\n", "") + " : " + (stderr + '').replace("\n", ""));
      } else {
        console.log("Finished " + param + " : " + (stdout + '').replace("\n", "") + " : " + (stderr + '').replace("\n", ""));
      }
      if (i === end - initial + 1) {
        createGif();
      } else if (queuedParameters.length > 0) {
        enqueueParameter(queuedParameters.splice(0,1)[0]);
      }
    });
  } else {
    queuedParameters.push(param);
  }
}

params.forEach(function(param, i) {
  if (!fs.existsSync(outdir + "/" + outfilebase + param[1] + ".png")) {
    enqueueParameter(param);
  } else {
    i++;
    if (i === (end - initial + 1)) {
      createGif();
    }
  }
});

