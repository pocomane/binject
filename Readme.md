
Binject
========

This is an utility for embed a script in an executable. It is designed as a
"Library" so you can provide the parsing function or manipulate the data before
the injection. A complete but simple "Echo" example is provided for reference.

All the code is in the Public Domain, so:
- You can do anything you want with it.
- You have no legal obligation to do anything else.
- There is NO WARRANTY of any kind.

NOTE: The code is still a bit messy, but it works.

Build
------

There is no actual build system. You can compile it with gcc using:

```
gcc -std=c99 -o binject.exe *.c
```

As it is, this will compile the "Echo" example. For some customization, read
the Options section.

Usage
------

When called without argument, some help information will be printed. To embed a
script pass it as argument.

```
./binject.exe my_script
```

This will generate the file my_script.exe. Then:

```
echo "hello world" > my_text.txt
./binject.exe my_text.txt
rm my_text.txt
./my_text.txt.exe
```

will print "hello world" to the screen.

In the test directory there is a test that will execute all the commands seen
before. At end it will also check that the output of the example app is the
expected one.

How it works
-------------

For now just two methods are avaiable to embed the script. By default, the
"Array" method will be tryed first, and if the script is too big, it will
fallback to the "Tail" method.

In the "Array" method the script will overwrite the initialization data of a
static array.

In the "Tail" method the script will be appended at end of the
executable, and in the static array will be kept only the informations
about where the script begin. With this method you can edit you script
directly in the exectuable.

For more details for both the methods, look at the Options section.

Options
--------

You can look at the top of the binject.h and binject.c for a full list
of compile-time options. A summary of the most important ones follows.

`BINJECT_ARRAY_SIZE` - Size of the data for the INTERNAL ARRAY
mechanism. It should be a positive integer. If you put this value to 0,
you can actually force to always use the tail method. The default is
9216 byte.

`VERB_LEVEL` - Write some information during the esecution. If it is > 9 also
some debug information is printed at every line (mainly position in the
sources). The default is 0, which means that nothing will be printed.

TODO : Describe other options

