
#ifndef __hacks_H
#define __hacks_H
// This file was created because clang doesn't support passing in a string as a MACRO value i.e. doing -DMACRO="\"some string\"" ends up passing in `some string`
// instead of `"some string"` - not sure if I should file a radar but oh well I'll add a stringify macro here

#define Z_STRINGIFY(a) #a

#endif

