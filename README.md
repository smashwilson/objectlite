Copyright (C) 2009 by Ashley J. Wilson and Roger E. Ostrander.
This software is licensed as described in the file COPYING in the root directory
of this distribution.

# What is ObjectLite?

ObjectLite is an attempt to provide an object database analog to SQLite: a fully
functional object database in a zero configuration, single, relocatable file.

The core library is written in C.  We intend to develop bindings to Ruby,
Smalltalk (VisualWorks), Python, and possibly C# and Smalltalk (Squeak/Pharo).

# Installation

To build from source, verify that you have the following prerequisites installed:

 * The [SCons build system](http://www.scons.org), which we're using instead of
   Makefiles.  Scons in turn depends on [Python](http://www.python.org).

 * [CUnit](http://cunit.sourgeforge.net/) for unit testing in C.

 * IBM's [ICU](http://site.icu-project.org/) Unicode library.

Have all of that?  Make sure that you have your newline settings correct (as
[discussed here](http://github.com/guides/dealing-with-newlines-in-git)) and
check out the latest code from github by cloning the repository:

`git clone git://github.com/smashwilson/objectlite.git`

Now enter the root directory of the git repository and type "scons".  Did that
work?  No?  Well, we're working on that.  If it did, you could run the
"unittests" executible and see how many tests we've written so far.