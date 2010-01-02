# Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
# This software is licensed as described in the file COPYING in the root
# directory of this distribution.
#
# = SConstruct master build file =
#
# Configures global environment settings and invokes one of the other SConstruct
# build files as appropriate.

import os
import sys

env = DefaultEnvironment(ENV = os.environ, tools = ['mingw'])

# Include path.
env.Append(CPPPATH = os.environ.get('CPATH'))
env.Append(CPPPATH = os.pathsep + 'obl')

# Library path.
env.Append(LIBPATH = os.environ.get('LD_LIBRARY_PATH'))
env.Append(LIBPATH = os.pathsep + 'obl')

env.Append(CCFLAGS = '-Wall')

# Handle command-line arguments. ###############################################

AddOption('--debug-build',
    action='store_true',
    dest='debug_build',
    default=False,
    help='Include debugging symbols within the binary.')
    
if GetOption('debug_build'):
    env.Append(CCFLAGS = '-g')

# libobjectlite: The core library. #############################################

obllib = StaticLibrary(
    'obl/objectlite',
    Glob('obl/*.c') + Glob('obl/storage/*.c'))

# unittests: Unit testing suite for libobjectlite. #############################

utlibs = ['objectlite', 'cunit', 'icuuc']
if sys.platform == 'win32':
    utlibs.append('ws2_32')

obltest = Program(
    'unittests',
    Glob('obl/test/*.c'),
    LIBS = utlibs)

# Documentation with doxygen. ##################################################

doctask = Command('doc/html/index.html', obllib, 'doxygen Doxyfile')

# Alias directives for common configurations. ##################################

Alias('lib', obllib)
Alias('test', obltest)
Alias('docs', doctask)

Default(obllib)

# Additional -c clean rules. ###################################################

for scratch in Glob('*~') + Glob('*/*~') + Glob('*/*/*~'):
    Clean(str(scratch)[:-1], scratch)

Clean(obllib, Glob('*.log'))
Clean(obllib, Glob('*.obl'))
Clean(obllib, Glob('doc/**'))
