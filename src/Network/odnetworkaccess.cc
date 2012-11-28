/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odnetworkaccess.h"

#include "file.h"
#include "filepath.h"
#include "odnetworkreply.h"
#include "qnetworkaccessconn.h"
#include "separstr.h"

#include <QEventLoop>
#include <QNetworkProxy>



ODNetworkAccess::ODNetworkAccess()
    : qnam_( new QNetworkAccessManager )
    , qeventloop_( new QEventLoop )
    , finished( this )
    , odnr_( 0 )
{
    qnamconn_ = new QNAMConnector( qnam_, this );
}


ODNetworkAccess::~ODNetworkAccess()
{
    delete qeventloop_;
    delete qnamconn_;
    delete qnam_;
}


void ODNetworkAccess::setHttpProxy( const char* hostname, int port, bool auth, 
				    const char* username, const char* password )
{
    QNetworkProxy proxy;
    proxy.setType( QNetworkProxy::HttpProxy );
    proxy.setHostName( hostname );
    proxy.setPort( port );
    if ( auth )
    {
	proxy.setUser( username );
	proxy.setPassword( password );
    }
}


bool ODNetworkAccess::getFile( const char* url, const char* path, 
								TaskRunner* tr )
{
    if ( File::isDirectory(path) )
    {
	BufferStringSet bss;
	bss.add( url );
	return getFiles ( bss, path, tr );
    }

    QNetworkReply* qnr = qnam_->head( QNetworkRequest(QUrl(url)) );
    ODNetworkReply* odnr = new ODNetworkReply( qnr );
    startEventLoop();
    od_int64 filesize = odnr->RemoteFileSize();
    if ( filesize == 0 )
    {
	FilePath fp(path);
	errormsg_.add( "Something went wrong while downloading the file: ");
	errormsg_.add( fp.fileName() );
	errormsg_.add( " \nPlease check if you are connected to internet.");
    }

    delete odnr;
    ODNetworkTask odnetworktask( qnam_, url, path );
    odnetworktask.setTotalNr( filesize );
    if ( tr )
    {
	if ( !tr->execute(odnetworktask) )
	{
	    errormsg_ = odnetworktask.message();
	    return false;
	}
    }
    else
    {
	if ( !odnetworktask.execute() )
	{
	    errormsg_ = odnetworktask.message();
	    return false;
	}
    }
    
    return  true;
}


bool ODNetworkAccess::getFiles( const BufferStringSet& urls, 
				const char* outputdir, TaskRunner* tr)
{
    ExecutorGroup egroup ( "Downloading Files" );
    ODNetworkTask* odnt = 0;
    for ( int idx=0; idx<urls.size(); idx++ )
    {
	SeparString str( urls.get(idx).buf(), '/' );
	if ( str.size() < 3 )
	    return false;

	FilePath destpath( outputdir );
	if ( str[str.size()-1].isEmpty() )
	    destpath.add( str[str.size() - 2] );
	else
	    destpath.add( str[str.size() - 1] );

	QNetworkReply* qnr = qnam_->head( QNetworkRequest(QUrl
						    (urls.get(idx).buf())) );
	ODNetworkReply* odnr = new ODNetworkReply( qnr );
	startEventLoop();
	od_int64 filesize = odnr->RemoteFileSize();
	if ( filesize == 0 )
	{
	    errormsg_.add( "Something went wrong while downloading the file: ");
	    errormsg_.add( destpath.fileName() );
	    errormsg_.add( " \nPlease check if you are connected to internet.");
	}

	delete odnr;
	odnt = new ODNetworkTask( qnam_, urls.get(idx).buf(), 
						destpath.fullPath() );
	odnt->setTotalNr( filesize );
	egroup.add( odnt );
    }

    if ( tr )
    {
	if ( !tr->execute(egroup) )
	{
	    errormsg_ = egroup.message();
	    return false;
	}
    }
    else
    {
	if ( !egroup.execute() )
	{
	    errormsg_ = egroup.message();
	    return false;
	}
    }

    return true;
}


bool ODNetworkAccess::finish( CallBacker* )
{ return true; }


void ODNetworkAccess::startEventLoop()
{ qeventloop_->exec(); }


void ODNetworkAccess::stopEventLoop()
{ qeventloop_->exit(); }


bool ODNetworkAccess::isEventLoopRunning()
{ return qeventloop_->isRunning(); }


int ODNetworkTask::nextStep()
{
    if ( init_ )
    { 
	init_ = false;
	qnetworkreply_ = qnam_->get( QNetworkRequest(QUrl(url_.buf())) );
	odnr_ = new ODNetworkReply( qnetworkreply_ );
    }

    odnr_->startEventLoop();
    if ( odnr_->replyStatus() == 1 )
    {
	if ( !readData() )
	{
	    delete odnr_;
	    return ErrorOccurred();
	}
    }


    if ( odnr_->replyStatus() == 2 )
    {
	osd_.close();
	delete odnr_;
	return Finished();
    }

    if ( odnr_->replyStatus() == 3 )
    {
	errorOccured();
	osd_.close();
	delete odnr_;
	return ErrorOccurred();
    }
    
    if ( nrdone_ < totalnr_ )
	return MoreToDo();
    else
    {
	osd_.close();
	delete odnr_;
	return  Finished();
    }
}


bool ODNetworkTask::readData()
{
    FilePath fp = outputdest_.buf();
    if ( nrdone_ == 0 )
	msg_ = fp.fileName();

    od_int64 bytes = qnetworkreply_->bytesAvailable();
    char* buffer = new char[bytes];
    qnetworkreply_->read( buffer, bytes );
    if ( !osd_.usable() )
    {
	if ( !File::exists(fp.pathOnly()) )
	    File::createDir( fp.pathOnly() );

	osd_ = StreamProvider( outputdest_ ).makeOStream();
	if ( !osd_.usable() )
	{
	    msg_.add( " Didn't have permission to write" );
	    return false;
	}
    }

    osd_.ostrm->write( buffer, bytes );
    nrdone_ += bytes;
    return true;
}


bool ODNetworkTask::errorOccured()
{
    if( qnetworkreply_->error() > 0 )
    {
	FilePath fp( outputdest_ );
	msg_ = "Something went wrong while downloading the file: ";
	msg_.add( fp.fileName() );
	msg_.add( " \nDetails: ");
	msg_ += qPrintable( qnetworkreply_->errorString() );
	return true;
    }

    return false;
}


const char* ODNetworkTask::message() const
{ return msg_; }


od_int64 ODNetworkTask::nrDone() const
{return nrdone_/1024;}


const char* ODNetworkTask::nrDoneText() const
{return "KBytes downloaded";}


od_int64 ODNetworkTask::totalNr() const
{ return totalnr_/1024; }


void ODNetworkTask::setTotalNr( const od_int64 totalnr )
{ totalnr_ = totalnr; }

