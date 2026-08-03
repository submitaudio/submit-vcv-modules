# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add only the modules approved for this release. Local beta sources remain in
# the repository for patch compatibility and development, but are deliberately
# excluded from the public release binary.
SOURCES += src/plugin.cpp
SOURCES += src/Drift13.cpp
SOURCES += src/Chrono.cpp
SOURCES += src/Impact.cpp
SOURCES += src/Chain.cpp
SOURCES += src/Squeeze.cpp
SOURCES += src/Shape.cpp
SOURCES += src/Master.cpp
SOURCES += src/Gain.cpp
SOURCES += src/Sweep.cpp
SOURCES += src/Loop.cpp
SOURCES += src/Clang.cpp
SOURCES += src/React.cpp
SOURCES += src/Sync.cpp
SOURCES += src/Flip.cpp
SOURCES += src/Orbit.cpp

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
