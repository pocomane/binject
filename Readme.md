
Binject
========

This is an utility for embed a script in an executable. It is designed as a
"Library" so you can provide the parsing function or manipulate the data before
the injection. A complete but simple "Echo" example is provided for reference.

This software is released under the [Unlicense](http://unlicense.org), so the
legal statement in COPYING.txt applies.

Build
------

There is no actual build system. You can compile it with gcc using:

```
gcc -std=c99 -o binject.exe *.c
```

As it is, this will compile the "Echo" example. For some customization, read
the 'How it works' section.

Usage
------

When called without argument, some help information will be printed. To embed a
script pass it as argument.

```
./binject.exe my_script
```

This will generate the file injed.exe. Then:

```
echo "hello world" > my_text.txt
./binject.exe my_text.txt
rm my_text.txt
./injed.exe
```

will print "hello world" to the screen.

In the test directory there is a test that will execute all the commands seen
before. At end it will also check that the output of the example app is the
expected one.

How it works
-------------

Two methods are avaiable to embed the script. By default, the "Array" method
will be tryed first, and if the script is too big, it will fallback to the
"Tail" method.

In the "Array" method the script will overwrite the initialization data of a
static struct.

In the "Tail" method the script will be appended at end of the
executable, and in the static struct will be kept only the informations
about where the script begin. With this method you can edit you script
directly in the exectuable.

The example application can be configured at compile time by means of the
following definitions.

`BINJECT_ARRAY_SIZE` - Size of the data for the INTERNAL ARRAY
mechanism. It should be a positive integer. If you put this value to 0,
you can actually force to always use the tail method. The default is
9216 byte.

