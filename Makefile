#
#  There exist several targets which are by default empty and which can be 
#  used for execution of your targets. These targets are usually executed 
#  before and after some main targets. They are: 
#
#     .build-pre:              called before 'build' target
#     .build-post:             called after 'build' target
#     .clean-pre:              called before 'clean' target
#     .clean-post:             called after 'clean' target
#     .clobber-pre:            called before 'clobber' target
#     .clobber-post:           called after 'clobber' target
#     .all-pre:                called before 'all' target
#     .all-post:               called after 'all' target
#     .help-pre:               called before 'help' target
#     .help-post:              called after 'help' target
#
#  Targets beginning with '.' are not intended to be called on their own.
#
#  Main targets can be executed directly, and they are:
#  
#     build                    build a specific configuration
#     clean                    remove built files from a configuration
#     clobber                  remove all built files
#     all                      build all configurations
#     help                     print help mesage
#  
#  Targets .build-impl, .clean-impl, .clobber-impl, .all-impl, and
#  .help-impl are implemented in nbproject/makefile-impl.mk.
#
#  Available make variables:
#
#     CND_BASEDIR                base directory for relative paths
#     CND_DISTDIR                default top distribution directory (build artifacts)
#     CND_BUILDDIR               default top build directory (object files, ...)
#     CONF                       name of current configuration
#     CND_PLATFORM_${CONF}       platform name (current configuration)
#     CND_ARTIFACT_DIR_${CONF}   directory of build artifact (current configuration)
#     CND_ARTIFACT_NAME_${CONF}  name of build artifact (current configuration)
#     CND_ARTIFACT_PATH_${CONF}  path to build artifact (current configuration)
#     CND_PACKAGE_DIR_${CONF}    directory of package (current configuration)
#     CND_PACKAGE_NAME_${CONF}   name of package (current configuration)
#     CND_PACKAGE_PATH_${CONF}   path to package (current configuration)
#
# NOCDDL


# Environment 
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
CURRENT_PLATFORM=$(shell uname -s)
CONF ?= Debug

ifndef KARABO
	KARABO := $(shell cat ${HOME}/.karabo/karaboFramework)
endif

export PKG_CONFIG_PATH=$(KARABO)/lib:$(KARABO)/extern/lib/pkgconfig

# build
build: .build-post

.build-pre: src/version.hh
	@echo "\n### INFO Building against karaboFramework: $(KARABO)\n"
# Add your pre 'build' code here...

.build-post: .build-impl
# Create platform-specific copies of the current makefiles generated by netbeans
	@$(CP) -fp $(CND_BASEDIR)/nbproject/Makefile-variables.mk $(CND_BASEDIR)/nbproject/Karabo-variables-$(CURRENT_PLATFORM).mk
	@$(CP) -fp $(CND_BASEDIR)/nbproject/Makefile-impl.mk $(CND_BASEDIR)/nbproject/Karabo-impl-$(CURRENT_PLATFORM).mk
	@$(CP) -fp $(CND_BASEDIR)/nbproject/Makefile-$(CONF).mk $(CND_BASEDIR)/nbproject/Karabo-$(CONF)-$(CURRENT_PLATFORM).mk

# Add your post 'build' code here...


# clean
clean: .clean-post

.clean-pre:
# Add your pre 'clean' code here...

.clean-post: .clean-impl
# Add your post 'clean' code here...


# clobber
clobber: .clobber-post

.clobber-pre:
# Add your pre 'clobber' code here...

.clobber-post: .clobber-impl
# Add your post 'clobber' code here...


# all
all: .all-post

.all-pre:
# Add your pre 'all' code here...

.all-post: .all-impl
# Add your post 'all' code here...


# build tests
build-tests: .build-tests-post

.build-tests-pre:
# Add your pre 'build-tests' code here...

.build-tests-post: .build-tests-impl
# Add your post 'build-tests' code here...


# run tests
test: .test-post

.test-pre:
# Add your pre 'test' code here...

.test-post: .test-impl
# Add your post 'test' code here...


# help
help: .help-post

.help-pre:
# Add your pre 'help' code here...

.help-post: .help-impl
# Add your post 'help' code here...

# create binary package
package: build
	@$(KARABO)/bin/.bundle-cppplugin.sh $(CND_DISTDIR) $(CONF) $(CND_PLATFORM_$(CONF))

# The following three targets are created by NetBeans. If we build using the
# command line, we copy back the platform-specific copies generated by the
# latest NetBeans build on this platform, otherwise they exist already.
# Copy option -p is relevant, otherwise the copies might look newer than a
# changed Karabo-*-$(CURRENT_PLATFORM), received from git update.
nbproject/Makefile-impl.mk: nbproject/Karabo-impl-$(CURRENT_PLATFORM).mk
	$(CP) -fp nbproject/Karabo-impl-$(CURRENT_PLATFORM).mk nbproject/Makefile-impl.mk

nbproject/Makefile-variables.mk: nbproject/Karabo-variables-$(CURRENT_PLATFORM).mk
	$(CP) -fp nbproject/Karabo-variables-$(CURRENT_PLATFORM).mk nbproject/Makefile-variables.mk

nbproject/Makefile-$(CONF).mk: nbproject/Karabo-$(CONF)-$(CURRENT_PLATFORM).mk
	$(CP) -fp nbproject/Karabo-$(CONF)-$(CURRENT_PLATFORM).mk nbproject/Makefile-$(CONF).mk

# The following two includes lead to warnings from make ("No such file or
# directory") when compiling the first time after a fresh checkout
# (see https://www.gnu.org/software/make/manual/html_node/Include.html).
# But then it will create the files according to the above rules. In fact, even
# if the files were not missing, they might need to be recreated before used!


# include project implementation makefile
include nbproject/Makefile-impl.mk

# include project make variables
include nbproject/Makefile-variables.mk

# nbproject/Makefile-$(CONF).mk is not included here, but is used in all
# the .*-impl targets used above (and defined in Makefile-impl.mk).
# To take care that it exists before the .validate-impl target in
# Makefile-impl.mk checks that, we add it here as a prerequisite.
# Since this target is really internal to the NetBeans generated
# Makefile-impl.mk, this should be considered a HACK around the fact that there
# the *-pre targets appear as prerequisites in parallel with .validate.impl,
# i.e. we cannot simply make the .*-pre targets depend on Makefile-$(CONF).mk
# here. That is because then the check might still be executed before the
# .*-pre target that leads to the creation of Makefile-$(CONF).mk.
# Hope that people reading this could follow...
.validate-impl: nbproject/Makefile-$(CONF).mk



PACKAGE_NAME=$(shell basename -s .git `git remote -v | grep fetch | head -n1 | awk '{ print $$2 }' `)

src/version.hh: .git/HEAD .git/index .git/refs/tags
	@echo "// WARNING: This file is auto generated by the Makefile." > $@
	@echo "#ifndef PACKAGE_VERSION" >> $@
	# Note that --dirty can be fooled: Build once when clean and then change source files - or vice a versa...
	@echo "#define PACKAGE_VERSION \"$(PACKAGE_NAME)-$(shell git describe --tags --match "*.*.*" --dirty --always )\"" >> $@
	@echo "#endif" >> $@
