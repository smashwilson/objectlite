# Copyright (C) 2009 Ashley J. Wilson, Roger E. Ostrander
# This software is licensed as described in the file COPYING in the root
# directory of this distribution.
#
# = SConstruct master build file =
#
# Configures global environment settings and invokes one of the other SConstruct
# build files as appropriate.

import os
env = DefaultEnvironment(ENV = os.environ, tools = ['mingw'])

# Include path.
env['CPPPATH'] = [
'C:/CUnit-2.1-0/', 'core'
]

# Library path.
env['LIBPATH'] = [
'C:/CUnit-2.1-0/VC8/Release - Static Lib', 'core'
]

# libobjectlite: The core library. #############################################
corelib = StaticLibrary(
    'core/objectlite',
    Glob('core/*.c'),
    LIBS = ['ws2_32'])

# unittests: Unit testing suite for libobjectlite. #############################
coretest = Program(
    'unittests',
    Glob('core/test/*.c'),
    LIBS = ['objectlite', 'cunit', 'ws2_32'])

# Alias directives for common configurations. ##################################
Alias('corelib', corelib)
Alias('coretest', coretest)

# Additional -c clean rules. ###################################################

for scratch in Glob('*~ */*~'):
    Clean(str(scratch)[:-1], scratch)

Clean('.', Glob('*.log'))
Clean('.', Glob('*.obl'))
