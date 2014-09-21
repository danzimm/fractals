# Fractals

Thanks to [@SquiffyPwn](https://twitter.com/squiffypwn) I decided to create some fractals. Since I'm me I decided to make it a small fractal creation library / tool. So here is the result - a cpp library along with a cli tool which takes plugins to create custom fractals.

# Docs

Check out the source of `fractalgen` to see how the program works, use `-z` to see the help. TL;DR:
> Specify `-k KERNEL` to specify the name of the opencl kernel to use. The program will look for `KERNEL.cl` and the kernel `KERNEL` inside that file to run the program with.

# TODO

- Create proper docs
  - Modify above to be more in depth
  - Create manpage
  - Create proper `-h` flag instead of `-z`
- Add code to save as other formats
  - Add code to allow this to output a CoreGraphics bitmap
- Add node backend to communicate to this library
- Add website frontend
- Linux compatibility (maybe windows...)
- Make this more modular so you don't have to have so much boilerplate code

# Some Notes

This uses opencl. At the moment it simply creates a png by tiling the image you want to create and rendering the tiles one at a time. Some weird errors occur if you specify too large a tile size (it seems like some sort of watchdog on the GPU kicks in and kills the kernel that's running - no idea how to fix this).

# Samples

Mandlebrot Set
![Mandlebrot Set](mandlebrotsample.png)

