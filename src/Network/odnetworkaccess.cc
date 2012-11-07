/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = " ";

#include "odnetworkaccess.h"
#include "odnetworkreply.h"
#include "qnetworkaccessconn.h"
#include "separstr.h"

#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QUrl>
#include <QString>
#include <QByteArray>



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


bool ODNetworkAccess::get( const char* url, const char* path, TaskRunner* tr )
{
    odnr_ = new ODNetworkReply( qnam_->get(QNetworkRequest(QUrl(url))) );
    odnr_->setDestPath( path );
    finished.notify( mCB(this,ODNetworkAccess,finish) );
    ODNetworkTask odnetworktask( this );
    odnr_->setODNetworkTask( odnetworktask );
    tr ? tr->execute(odnetworktask) : startEventLoop();
    finished.remove( mCB(this,ODNetworkAccess,finish) );
    delete odnr_;
    if ( errormsg_.size() != 0 )
	return false;

    return  true;
}


bool ODNetworkAccess::getFiles( const BufferStringSet& urls, 
				const char* outputdir, TaskRunner* tr)
{
    ODNATask exec( this, urls, outputdir );
    if ( !(tr ? tr->execute(exec) : exec.execute()) )
	return false;

    return true;
}


bool ODNetworkAccess::finish( CallBacker* )
{
    errormsg_ = odnr_->errorMsg();
    if ( errormsg_.size()!=0 )
	return false;

    return true;
}


void ODNetworkAccess::startEventLoop()
{ qeventloop_->exec(); }


void ODNetworkAccess::stopEventLoop()
{ qeventloop_->exit(); }


int ODNATask::nextStep()
{
    SeparString str( urls_.get(nrdone_).buf(), '/' );
    if ( str.size() < 3 )
    {
	return false;
    }

    BufferString destpath = outputdir_;
    destpath += str[ str.size() - 1 ];
    if ( !(odna_->get(urls_.get(nrdone_).buf(),destpath)) )
	return ErrorOccurred();

    nrdone_++;
    if ( nrdone_ < totalNr() )
	return MoreToDo();

    return Finished();
}


const char* ODNATask::message() const
{ return odna_->errorMsg(); }


od_int64 ODNATask::nrDone() const
{return nrdone_;}


const char* ODNATask::nrDoneText() const
{return "Files";}


od_int64 ODNATask::totalNr() const
{ return urls_.size(); }


int ODNetworkTask::nextStep()
{
    odna_->startEventLoop();
    return  true;
}


const char* ODNetworkTask::message() const
{ return 0; }


od_int64 ODNetworkTask::nrDone() const
{return nrdone_;}


const char* ODNetworkTask::nrDoneText() const
{return "%";}


od_int64 ODNetworkTask::totalNr() const
{ return totalnr_; }
