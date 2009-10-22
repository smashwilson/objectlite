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
env.Append(CPPPATH = os.environ.get('INCLUDE'))
env.Append(CPPPATH = ';obl')

# Library path.
env.Append(LIBPATH = os.environ.get('LIB'))
env.Append(LIBPATH = ';obl')

env.Append(CCFLAGS = '-g');

# libobjectlite: The core library. #############################################
obllib = StaticLibrary(
    'obl/objectlite',
    Glob('obl/*.c') + Glob('obl/storage/*.c'))

# unittests: Unit testing suite for libobjectlite. #############################
obltest = Program(
    'unittests',
    Glob('obl/test/*.c'),
    LIBS = ['objectlite', 'cunit', 'ws2_32', 'icuuc'])

# Documentation with doxygen. ##################################################

doctask = Command('doc/html/index.html', obllib, 'doxygen Doxyfile')

# Alias directives for common configurations. ##################################
Alias('lib', obllib)
Alias('test', obltest)
Alias('docs', doctask)

# Additional -c clean rules. ###################################################

for scratch in Glob('*~') + Glob('*/*~') + Glob('*/*/*~'):
    Clean(str(scratch)[:-1], scratch)

Clean('.', Glob('*.log'))
Clean('.', Glob('*.obl'))
Clean('.', Glob('doc/**'))
