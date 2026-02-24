################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Bluetooth LE Throughput Course - Peripheral Firmware
#
################################################################################

################################################################################
# Basic Configuration
################################################################################

MTB_TYPE=COMBINED
MTB_PROBE_SERIAL=0A1803E8021D2400

# Target board/hardware (BSP).
TARGET=CYW920829M2EVK-02

# Name of application (used to derive name of final linked file).
APPNAME=lab2-solution-peripheral

# Toolchain to use.
TOOLCHAIN=GCC_ARM

# Build configuration (Debug or Release).
CONFIG=Debug

# Display full command-lines when building.
VERBOSE=

################################################################################
# Advanced Configuration
################################################################################

# Enable optional code components.
COMPONENTS=FREERTOS WICED_BLE

# Disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# Manually add source code to the build process.
SOURCES=

# Additional include directories.
INCLUDES=./configs

# Additional defines for the build process.
DEFINES=CY_RETARGET_IO_CONVERT_LF_TO_CRLF CY_RTOS_AWARE STACK_INSIDE_FREE_RTOS

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
CFLAGS=

# Additional / custom C++ compiler flags.
CXXFLAGS=

# Additional / custom assembler flags.
ASFLAGS=

# Additional / custom linker flags.
LDFLAGS=

# Additional / custom libraries to link in to the application.
LDLIBS=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=

################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
CY_APP_PATH=

# Relative path to the shared repo location.
CY_GETLIBS_SHARED_PATH=../../../

# Directory name of the shared repo location.
CY_GETLIBS_SHARED_NAME=mtb_shared

# Absolute path to the compiler's "bin" directory.
CY_COMPILER_PATH=

# Locate ModusToolbox IDE helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    $(HOME)/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)

# If you install ModusToolbox IDE in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder). Make sure you use forward slashes.
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS). On Windows, use forward slashes.)
endif

$(info Tools Directory: $(CY_TOOLS_DIR))

include $(CY_TOOLS_DIR)/make/start.mk
