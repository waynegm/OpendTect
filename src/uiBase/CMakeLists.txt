#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic UI-classes
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.7 2012-09-07 22:08:04 cvsnanne Exp $
#_______________________________________________________________________________

set( OD_MODULE_DEPS General)
set( OD_USEQT Widgets)
set( QT_MOC_HEADERS
		i_qbutton.h
		i_qobjectptr.h
	)
set( OD_MODULE_SOURCES
		pixmap.cc
		uibaseobject.cc
		uibutton.cc
		uidialog.cc
		uieventfilter.cc
		uigroup.cc
		uilabel.cc
		uimain.cc
		uirgbarray.cc
		uiwindowbase.cc
		
	)
set( OD_MODULE_GUI_PROGS od_test_uiBase.cc )
OD_INIT_MODULE()
