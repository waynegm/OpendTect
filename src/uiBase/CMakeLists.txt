#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic UI-classes
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.7 2012-09-07 22:08:04 cvsnanne Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS General)
SET(OD_USEQT Gui)
SET(QT_MOC_HEADERS  )
SET(OD_MODULE_SOURCES  
		uibaseobject.cc
		uidialog.cc
		uigroup.cc
		uilabel.cc
	)
SET(OD_MODULE_GUI_PROGS )
OD_INIT_MODULE()
