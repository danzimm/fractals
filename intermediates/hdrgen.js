
var fs    = require('fs'),
    path  = require('path'),
    filename, filewoext, data, base_ptx, default_colorizer;

var filename = "ptx.h";
if (process.argv.length >= 3) {
  filename = path.resolve(process.argv[2], filename);
}

var files = ["escape", "cgame"];

var filewoext = path.basename(filename, path.extname(filename)).toLowerCase();
var data = "// Autogenerated by hdrgen.js\n\n#ifndef __" + filewoext + "_h\n#define __" + filewoext + "_h\n\n";

function append_files_to_header(files, type, dat) {
  var arrdata = "const char *" + type + "_ptxs[] = {\n";
  files.forEach(function(file, idx) {
    file = file + "_" + type;
    dat += "const char " + file + "[] = \"\\n\\\n";
    var tmp = fs.readFileSync(file + ".ptx", "utf8");
    tmp = tmp.split("\n").join("\\n\\\n").split("\"").join("\\\"");
    dat += tmp + "\";\n\n";
    arrdata += "  " + file + ",\n";
  });
  dat += arrdata.substring(0, arrdata.length - 2) + "\n};\n\n";
  return dat;
}

data = append_files_to_header(files, "base", data);
data = append_files_to_header(files, "colorizer", data);

data += "unsigned number_fractal_types = " + files.length + ";\n\n";

data += "#endif\n\n";

fs.writeFileSync(filename, data);

