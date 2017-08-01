
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

In the "Tail" method the script will be appended at end of the executable.
After the script a tag line will be added also. It describe where the script
starts but if you delete it, the beginning will be just searched from the
bottom. With this method you can edit you script directly in the exectuable.

For more details for both the methods, look at the Options section.

Options
--------

You can look at the top of the binject.h and binject.c for a full list of
compile-time options. For example you can set the parsing function or you can
disable the internal main function, and use binject as a library. A summary of
the most important ones follows.

`BINJECT_SCRIPT_HANDLER` - If the main function is enabled, this callback will
be called to execute the script. It must be a function with the following
prototype: `int my_run_callback(binject_info_t * info, int argc, char **argv)`.
The default is a function that simply print the embedded content to the screen.

`BINJECT_SCRIPT_INJECT` - If the main function is enabled, this callback will
be called to get the data to be embedded. E.g. it is usefull if you want to
compress the code befor the inclusion. It must be a function with the following
prototype: `void my_inj_callback(binject_info_t * info, char * scr_path, char *
out_path)`.  The default is a function that just copy the file at argv[1].

`BINJECT_ARRAY_SIZE` - Size of the data for the INTERNAL ARRAY mechanism. It
should be a positive integer. The default is 9216 byte.

`BINJECT_COMMENT_START` and `BINJECT_COMMENT_END` - When in "Tail" mode these
flags are used to format the last line as a script comment line. The default is
a lua style comment.

TODO : Describe how to force a specific mechanism

