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


FileDownloader::FileDownloader(const char* url,const char* path)
    : qeventloop_(new QEventLoop())
    , odnr_(0)
    , init_(true)
    , msg_(0)
    , nrdone_(0)
    , nrfilesdownloaded_(0)
    , totalnr_(0)
{
    SeparString str( url, '/' );
    FilePath destpath( path );
    if ( str[str.size()-1].isEmpty() )
	destpath.add( str[str.size() - 2] );
    else
	destpath.add( str[str.size() - 1] );

    files_.add( url );
    outputpath_.add( destpath.fullPath() );
}


FileDownloader::FileDownloader( const BufferStringSet& urls, 
				const char* path )
    : qeventloop_(new QEventLoop())
    , odnr_(0)
    , files_( urls )
    , init_(true)
    , msg_(0)
    , nrdone_(0)
    , nrfilesdownloaded_(0)
    , totalnr_(0)
{
    for ( int idx=0; idx<files_.size(); idx++ )
    {
	SeparString str( urls.get(idx).buf(), '/' );
	FilePath destpath( path );
	if ( str[str.size()-1].isEmpty() )
	    destpath.add( str[str.size() - 2] );
	else
	    destpath.add( str[str.size() - 1] );

	files_.add( urls.get(idx) );
	outputpath_.add( destpath.fullPath() );
    }
}


FileDownloader::~FileDownloader()
{ delete qeventloop_; }


int FileDownloader::nextStep()
{
    if ( totalnr_ == 0 )
    {
	getFileSize();
	if ( totalnr_ == 0 )
	    return -1;
	return MoreToDo();
    }

    if ( init_ )
    { 
	init_ = false;
	if ( !files_.validIdx( nrfilesdownloaded_ ) )
	    return Finished();
	
	odnr_ = new ODNetworkReply( ODNA().get(QNetworkRequest(QUrl(files_.get
				(nrfilesdownloaded_).buf()))), qeventloop_ );
    }

    qeventloop_->exec();
    if ( odnr_->qNetworkReply()->isFinished() )
    {
	osd_.close();
	init_ = true;
	nrfilesdownloaded_++;
	delete odnr_;
    }
    else if ( odnr_->qNetworkReply()->error() )
    {
	errorOccured();
	osd_.close();
	delete odnr_;
	return ErrorOccurred();
    }
    else if ( odnr_->qNetworkReply()->bytesAvailable() )
    {
	if ( !readData() )
	{
	    delete odnr_;
	    return ErrorOccurred();
	}
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


void FileDownloader::getFileSize()
{
    totalnr_ = 0;
    for ( int idx=0; idx<files_.size(); idx++ )
    {
	QNetworkReply* qnr = ODNA().head( QNetworkRequest
						(QUrl(files_.get(idx).buf())) );
	ODNetworkReply* odnr = new ODNetworkReply( qnr, qeventloop_ );
	qeventloop_->exec();
	od_int64 filesize = odnr->qNetworkReply()->header
			       ( QNetworkRequest::ContentLengthHeader ).toInt();
	totalnr_ += filesize;
	delete odnr;
    }
}


bool FileDownloader::readData()
{
    FilePath fp = outputpath_.get(nrfilesdownloaded_).buf();
    od_int64 bytes = odnr_->qNetworkReply()->bytesAvailable();
    char* buffer = new char[bytes];
    odnr_->qNetworkReply()->read( buffer, bytes );
    if ( !osd_.usable() )
    {
	if ( !File::exists(fp.pathOnly()) )
	    File::createDir( fp.pathOnly() );

	osd_ = StreamProvider( outputpath_.get(nrfilesdownloaded_) ).
								  makeOStream();
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


bool FileDownloader::errorOccured()
{
    if( odnr_->qNetworkReply()->error() > 0 )
    {
	FilePath fp( outputpath_.get(nrfilesdownloaded_) );
	msg_ = "Something went wrong while downloading the file: ";
	msg_.add( fp.fileName() );
	msg_.add( " \nDetails: ");
	msg_ += qPrintable( odnr_->qNetworkReply()->errorString() );
	return true;
    }

    return false;
}


const char* FileDownloader::message() const
{ return msg_; }


od_int64 FileDownloader::nrDone() const
{return nrdone_/1024;}


const char* FileDownloader::nrDoneText() const
{return "KBytes downloaded";}


od_int64 FileDownloader::totalNr() const
{ return totalnr_/1024; }


bool getFile( const char* url, const char* path, BufferString& errmsg, 
								TaskRunner* tr)
{
    FileDownloader dl( url, path );
    const bool res = tr ? tr->execute( dl ) : dl.execute();
    if ( !res ) errmsg = dl.message();
    return res;
}


bool getFiles( BufferStringSet& urls, const char* path, BufferString& errmsg, 
								TaskRunner* tr)
{
    FileDownloader dl( urls, path );
    const bool res = tr ? tr->execute( dl ) : dl.execute();
    if ( !res ) errmsg = dl.message();
    return res;
}


void setHttpProxy( const char* hostname, int port, bool auth, 
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

    QNetworkProxy::setApplicationProxy(proxy);
}


QNetworkAccessManager& ODNA()
{
    static Threads::Mutex lock;
    lock.lock();

    static QNetworkAccessManager* odna = 0;
    if ( !odna )
	odna = new QNetworkAccessManager();

    lock.unLock();
    return *odna;
}