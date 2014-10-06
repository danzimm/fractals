# Fractals

Thanks to [@SquiffyPwn](https://twitter.com/squiffypwn) I decided to create some fractals. Since I'm me I decided to make it a small fractal creation library / tool. So here is the result - ~~a cpp library~~ along with a cli tool which takes plugins to create custom fractals (the library no longer exists, I will eventually rip it out of the cli tool but as of now it doesn't exist).

# Docs

Check out the source of `fractalgen` to see how the program works, use `-z` to see the help. TL;DR:
> Specify `-p SUBKERNEL.ptx` to specify the name of the cuda file to use that has a `processPixel` function in it.

# TODO

- Create proper docs
  - Modify above to be more in depth
  - Create manpage
  - Proper docs on how to create these `ptx` files
- Add code to save as other formats
  - Add code to allow this to output a CoreGraphics bitmap
- Add node backend to communicate to this library
- Add website frontend
- Linux compatibility - mostly done
- Make this more modular so you don't have to have so much boilerplate code
- Add more boilerplate code to the `base_ptx.cu` file (i.e. add in the while loop and the such)

# Some Notes

This uses cuda. If you want to run this you will need two things: a cuda capable graphics card (I'm unsure at the moment which APIs I rely on but it works on my GEForce GT 650M so the requirements can't be too steep) and the cuda development kit installed. Fortunately for mac this means just installing the PKG from their website, otherwise on linux you'll need to turn off your window server (afaik) in order to properly install all the cuda components (this includes a graphics driver, that's why the window server needs to be turned off).

# Samples

Mandlebrot Set
![Mandlebrot Set](mandlebrotsample.png)

