# AMREX_HOME defines the directory in which we will find all the BoxLib code
AMREX_HOME ?= /project/projectdirs/nyx/src/amrex

HPGMG_DIR ?= /global/homes/f/friesen/hpgmg/finite-volume

# TOP defines the directory in which we will find Source, Exec, etc
TOP = ../..

DEFINES += -DAMREX_USE_CATALYST
# If we want to output image data when AMR refinement is disabled and number of blocks/grids equals number of MPI ranks
#DEFINES += -DCATALYST_IMAGEDATA

# result from ./paraview-config --include vtkPVPythonCatalyst
INCLUDE_LOCATIONS += /data/acbauer/Code/ParaView/debuginstall/include/paraview-5.4
#export LD_LIBRARY_PATH=/data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4

XTRALIBS += -Wl,-rpath-link,/data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPVPythonCatalyst-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPVCatalyst-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPVServerManagerApplication-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPVServerManagerCore-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPVServerImplementationCore-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPVClientServerCoreCore-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkFiltersParallel-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkFiltersExtraction-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkFiltersModeling-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkFiltersGeometry-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkFiltersProgrammable-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPVVTKExtensionsCore-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPVCommon-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkClientServer-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkIOXMLParser-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkFiltersSources-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkFiltersGeneral-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkFiltersCore-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkIOImage-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libprotobuf.so -lpthread /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkjsoncpp-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkPythonInterpreter-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkWrappingPython27Core-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkParallelMPI-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkParallelCore-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkIOLegacy-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkIOCore-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkCommonExecutionModel-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkCommonDataModel-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkCommonSystem-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtksys-pv5.4.so.1 -ldl /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkCommonMisc-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkCommonTransforms-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkCommonMath-pv5.4.so.1 /data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4/libvtkCommonCore-pv5.4.so.1  -Wl,-rpath,/data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4 -Wl,-rpath-link,/data/acbauer/Code/ParaView/debuginstall/lib/paraview-5.4

# compilation options
COMP    = gnu
USE_MPI = TRUE
USE_OMP = FALSE

PROFILE       = FALSE
TRACE_PROFILE = FALSE
COMM_PROFILE  = FALSE

PRECISION = DOUBLE
DEBUG     = FALSE

GIMLET = FALSE
REEBER = FALSE

GIMLET_DIR ?= /home/vince/Development/gimlet
# Gimlet needs FFTW MPI.
FFTW_INC ?= /usr/include
FFTW_DIR ?= /usr/lib/x86_64-linux-gnu

REEBER_HOME ?= /project/projectdirs/nyx/ghweber/reeber2
# Reeber needs Boost (both headers and libraries) and diy2.
BOOST_INCLUDE_DIR ?= /project/projectdirs/nyx/ghweber/boost-1.61.0-noarch/include
DIY_INCLUDE_DIR ?= /project/projectdirs/nyx/ghweber/diy/include

USE_HPGMG = FALSE
HPGMG_FCYCLES = FALSE
HPGMG_POST_F_CYCLE_TYPE = V
HPGMG_HELMHOLTZ = FALSE
HPGMG_STENCIL_VARIABLE_COEFFICIENT = FALSE
HPGMG_USE_SUBCOMM = TRUE
HPGMG_BOTTOM_SOLVER= BICGSTAB
HPGMG_SMOOTHER = GSRB

# physics
DIM      = 3
USE_GRAV = TRUE
USE_HEATCOOL = TRUE

USE_CVODE = FALSE

Bpack := ./Make.package
Blocs := .

include $(TOP)/Exec/Make.Nyx

