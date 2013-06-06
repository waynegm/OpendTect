/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "genc.h"
#include "bufstring.h"
#include "file.h"

# ifdef __msvc__
#  include "winmain.h"
# endif

#include <QtGui/QApplication>

#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgQt/GraphicsWindowQt>

#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgManipulator/TabBoxDragger>

#include <osgDB/ReadFile>

#include <iostream>

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    if ( argc!=2 )
    {
	std::cout << "Syntax: " << argv[0] << " <filename>\n";
	return 1;
    }
    
    BufferString file = argv[1];

    if ( !File::exists(file) )
    {
	std::cout << "File " << file.buf() << " could not be found.\n";
	return 1;
    }
    
    QApplication app(argc, argv);    
    osgQt::initQtWindowingSystem();

    osg::Node* root = osgDB::readNodeFile( file.buf() );

    if ( !root )
    {
	std::cout << "File " << file.buf() << " could not be read.\n";
	return 1;
    }
    
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->setSceneData( root );
    viewer->setCameraManipulator( new osgGA::TrackballManipulator );
    osgQt::setViewer( viewer.get() );
    
    osgQt::GLWidget* glw = new osgQt::GLWidget;
    osgQt::GraphicsWindowQt* graphicswin = new osgQt::GraphicsWindowQt( glw );
    
    viewer->getCamera()->setViewport(
		    new osg::Viewport(0, 0, glw->width(), glw->height() ) );
    viewer->getCamera()->setGraphicsContext( graphicswin );

    glw->show();

    return app.exec();
}
