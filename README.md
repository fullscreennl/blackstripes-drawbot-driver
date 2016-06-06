sketchy
=======

sketchy drawbot


### Cross compilation

In order to succesfully compile raspberry pi excutables we have create a crosscompile target in the make files.
Our cross-compiling machine has this cross compiling tools installed:

####Toolchain
git clone https://github.com/raspberrypi/tools

You can then copy the toolchain to a common location such as 
/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian, and add 
/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin to 
your $PATH in the .bashrc in your home directory. 
For 64bit, use /tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin. 
While this step is not strictly necessary, it does make it easier for later command lines!

To find out the lib path of this cross-coompiler you need to do this

$ arm-linux-gnueabihf-gcc -print-sysroot

this show where to look for libc .

also:

$ arm-linux-gnueabihf-gcc -print-search-dirs
 
You need to have rpi compiled libs of xenomai and native. I got them from a xenomaied Raspberry pi from /usr/lib/

Then you need to copy these libnative.* and libxenomai.* files into this sysroot in the folder 
[sysroot compiler]/usr/lib/ 

That together with some modified targets for using the right compiler include path and yo are ready to roll.

