#
# Core Flight Software Mission top-level CMake build script
# This will build cFS for all target machine(s) defined by the mission
#
# Note that the target CPUs may use different architectures, therefore each
# architecture must be done as a separate sub-build since none of the binaries 
# can be shared.
#
# This is actually two build scripts in one:
#   - A "top-level" script which divides the overall build by architecture
#       (This is run when TARGETSYSTEM is unset)
#   - An architecture-specific build that generates the binaries 
#       (This is run when TARGETSYSTEM is set)
#


# The minimum CMake version is chosen because 2.6.4 is what is
# included by default with RHEL/Centos 5.x
cmake_minimum_required(VERSION 2.6.4)

# This top-level tier does not define ANY targets but we know
# that the subprojects will at least use the "C" language, so
# indicate that now.  Doing this early initializes the CFLAGS
# so they won't change later.
project(ICAROUS_CFS C CXX)

OPTION(SITL "Build the project for SITL" OFF)
IF(SITL)
    add_definitions(-DSITL)
ENDIF(SITL)

set(CMAKE_CXX_FLAGS "-g -fPIC -m32")
set(CMAKE_C_FLAGS "-g -fPIC -m32")

# Add modules
add_subdirectory(Modules/ACCoRD)
add_subdirectory(Modules/FlightDataManager)
add_subdirectory(Modules/Plexil)
add_subdirectory(Modules/TrafficMonitor)
add_subdirectory(Modules/TrajectoryPlanners)
add_subdirectory(Modules/GeofenceMonitor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMESSAGE_FORMAT_IS_CCSDS")

IF (NOT DEFINED ENV{OSAL_HOME})
    message(FATAL_ERROR "$OSAL_HOME not defined but needed for compilation")
ENDIF ()

# Squelch a warning when building on Win32/Cygwin
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/cFS/bin")

set(OSAL_USER_C_FLAGS "-g")

set(CMAKE_CXX_FLAGS "-g -Wall -fPIC -D_LINUX_OS_ -D_HAVE_STDINT_ -D_XOPEN_SOURCE=600 -std=c++11")


# Force all builds to be 32 bit
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c99")

set(MISSIONCONFIG "Icarous")

# Include the global routines
include("CMake/global_functions.cmake")

# call the setup_globals() function which determines
# the correct mission definitions path (among other things)
setup_globals()

# Load the target configuration information (used by all builds)
# This is at the top level so all vars set in here will become globals.
include(CMake/targets.cmake)
    
# Scan the list of targets and organize by target system type.
read_targetconfig()

# load a sub-script that defines the other functions, depending on whether
# the TARGETSYSTEM is defined or not

# To use the double build system when building for multiple targets (e.g cpu1 & cpu2):
# 1. Uncomment this block
# 2. Comment out the next block
# 3. See mission_build.cmake and comment/uncomment the appropriate lines.

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#if(TARGETSYSTEM)
  #generate_config_includefile("inc/cfe_mission_cfg.h" mission_cfg.h ${MISSIONCONFIG} )
  #set(ARCH_BINARY_DIR "${CMAKE_BINARY_DIR}")
  #include("cfe/cmake/arch_build.cmake")
#else(TARGETSYSTEM)
  #include("cfe/cmake/mission_build.cmake")
#endif(TARGETSYSTEM)

#<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# Comment this block for multiple targets
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

include("CMake/mission_build.cmake")

# The prepare function should set up any necessary prerequisites for the build
if (COMMAND prepare)
  prepare()
endif (COMMAND prepare)

# Call the process_arch macro for each architecture
foreach(SYSVAR ${TGTSYS_LIST})
  process_arch(${SYSVAR})
endforeach(SYSVAR IN LISTS TGTSYS_LIST)

set(TARGETSYSTEM "cpu1")
set(ARCH_BINARY_DIR "${CMAKE_BINARY_DIR}")
include("CMake/arch_build.cmake")
#<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

if (COMMAND prepare)
    prepare()
endif (COMMAND prepare)

# Call the process_arch macro for each architecture
foreach(SYSVAR ${TGTSYS_LIST})
    process_arch(${SYSVAR})
endforeach(SYSVAR IN LISTS TGTSYS_LIST)

# The complete macro will do whatever is needed to finalize the build targets
if (COMMAND complete)
  complete()
endif (COMMAND complete)

