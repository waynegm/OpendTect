#_______________________Pmake___________________________________________________
#
#	Makefile: 	MATLAB plugin
# 	Feb 2012	Nanne
#	RCS:		$Id$
#_______________________________________________________________________________

SET( OD_MODULE_DEPS VolumeProcessing )
SET( OD_IS_PLUGIN yes )
SET( OD_MODULE_SOURCES
	multiinputpi.cc
	multiinputstep.cc
)

SET( OD_PLUGIN_ALO_EXEC ${OD_VOLUME_EXECS} )
OD_INIT_MODULE()
