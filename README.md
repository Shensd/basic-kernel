# Basic Kernel

"It's Your Only Option!" - Jack Hance, Creator of JackOS

## What is it?

This is just a basic kernel written in C and Assembly, pretty much all the complicated stuff is from [this tutorial](http://arjunsreedharan.org/post/82710718100/kernel-101-lets-write-a-kernel) and I have just made a few personal additions. It is built to run off of GRUB and can also be run with QEMU.

## What does it look like?

![Sample image of JackOS](https://i.imgur.com/hsyBq9N.png "Sample image of JackOS")

JackOS has been made with all the delicious styling of a hacker's terminal on a daytime TV show.

## What does it do?

As it stands right now, it just flashes some pretty text and has a prompt that you can type into, typing into does nothing because that hasn't been implemented yet (I'll get to it!). Future plans include:
 * Actual command functionality
 * Environment Variables
 * More fun hacker text
 * And many more!

Admittedly, I don't plan to get very far in the development of this, considering it's more of just a joke kernel. You never know though, maybe soon JackOS *will be the only choice*. 

## How to run it?

Even though this was built with the intent to run on GRUB, it works best on [QEMU](https://www.qemu.org/). It can be run with the very basic command: `qemu-system-i386 -kernel kernel` or by simply using `make run` from the makefile. The makefile also includes the command `make export` which should export it to the actual grub directory, where the the entry 
```
menuentry 'JackOS' {
    set root='hd2,msdos5'
    multiboot /boot/kernel-701 ro
}
```
needs to be added to `grub.cfg`. One thing to note: root needs to be changed to whatever your partition is that the kernel is stored on, this can be found by booting grub into command line mode and typing in `root (hd` and then hitting tab. 

###### This projct is licensed under the MIT Open Source license, see LICENSE for more information
