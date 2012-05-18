#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInternal.cmake,v 1.5 2012-05-18 10:14:02 cvskris Exp $
#_______________________________________________________________________________


#Configure odversion.h
configure_file ( ${OpendTect_DIR}/include/Basic/odversion.h.in ${OpendTect_DIR}/include/Basic/odversion.h )

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN "CVS" EXCLUDE )

#Install plugin example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/pluginexample DESTINATION doc/Programmer
	 PATTERN "CVS" EXCLUDE )

#Install data
install ( DIRECTORY "data" DESTINATION . PATTERN "CVS" EXCLUDE )
