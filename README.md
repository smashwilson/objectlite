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

 * The SCons build system, which we're using instead of Makefiles, available for
   download at http://www.scons.org/ .  Scons in turn depends on Python.

 * CUnit for unit testing in C.  Download and build from
   http://cunit.sourceforge.net/ .

 * IBM's ICU Unicode library: http://site.icu-project.org/ .

Have all of that?  Make sure that you have your newline settings correct (as
discussed at http://github.com/guides/dealing-with-newlines-in-git ) and check
out the latest code from github by cloning the repository:

git clone git://github.com/smashwilson/objectlite.git

Now enter the root directory of the git repository and type "scons".  Did that
work?  No?  Well, we're working on that.  If it did, you could run the
"unittests" executible and see how many tests we've written so far.