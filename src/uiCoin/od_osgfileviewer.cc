/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : May 2000
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "genc.h"
#include "bufstring.h"
#include "file.h"

#include <QtGui/QApplication>
#include <QtGui/QGridLayout>

#include <osgViewer/Viewer>

#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>

#include <osgDB/ReadFile>

#include <osgQt/GraphicsWindowQt>

#include <iostream>


#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QGridLayout>

#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>

#include <osgDB/ReadFile>

#include <osgQt/GraphicsWindowQt>

#include <iostream>

class ViewerWidget : public QWidget, public osgViewer::CompositeViewer
{
public:
    ViewerWidget(const char* filename) : QWidget()
    {
        setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);

        QWidget* widget1 = addViewWidget( createCamera(0,0,100,100), osgDB::readNodeFile(filename) );
        
        QGridLayout* grid = new QGridLayout;
        grid->addWidget( widget1, 0, 0 );
        setLayout( grid );

        connect( &_timer, SIGNAL(timeout()), this, SLOT(update()) );
        _timer.start( 10 );
    }
    
    QWidget* addViewWidget( osg::Camera* camera, osg::Node* scene )
    {
        osgViewer::View* view = new osgViewer::View;
        view->setCamera( camera );
        addView( view );
        
        view->setSceneData( scene );
        view->addEventHandler( new osgViewer::StatsHandler );
        view->setCameraManipulator( new osgGA::TrackballManipulator );
        
        osgQt::GraphicsWindowQt* gw = dynamic_cast<osgQt::GraphicsWindowQt*>( camera->getGraphicsContext() );
        return gw ? gw->getGLWidget() : NULL;
    }
    
    osg::Camera* createCamera( int x, int y, int w, int h, const std::string& name="", bool windowDecoration=false )
    {
        osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->windowName = name;
        traits->windowDecoration = windowDecoration;
        traits->x = x;
        traits->y = y;
        traits->width = w;
        traits->height = h;
        traits->doubleBuffer = true;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
        
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setGraphicsContext( new osgQt::GraphicsWindowQt(traits.get()) );
        
        camera->setClearColor( osg::Vec4(0.2, 0.2, 0.6, 1.0) );
        camera->setViewport( new osg::Viewport(0, 0, traits->width, traits->height) );
        camera->setProjectionMatrixAsPerspective(
            30.0f, static_cast<double>(traits->width)/static_cast<double>(traits->height), 1.0f, 10000.0f );
        return camera.release();
    }
    
    virtual void paintEvent( QPaintEvent* event )
    { frame(); }

protected:
    
    QTimer _timer;
};


int main( int argc, char** argv )
{

    SetProgramArgs( argc, argv );
    if ( argc!=2 )
    {
	std::cout << "Syntax: " << argv[0] << " <filename>\n";
	return 1;
    }

    
    QApplication app(argc, argv);

    BufferString file = argv[1];

    if ( !File::exists(file) )
    {
	std::cout << "File " << file.buf() << " could not be found.\n";
    }

    ViewerWidget* viewWidget = new ViewerWidget( file.buf() );
    viewWidget->setGeometry( 100, 100, 800, 600 );
    viewWidget->show();

    return app.exec();
}
