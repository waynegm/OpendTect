/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		October 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "file.h"
#include "filepath.h"
#include "odnetworkreply.h"
#include "strmprov.h"
#include "qnetworkaccessconn.h"

#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>


ODNetworkReply::ODNetworkReply( QNetworkReply* qnr )
    : downloadProgress( this )
    , finished( this )
    , metaDataChanged( this )
    , error( this )
    , uploadProgress( this )
    , aboutToClose( this )
    , bytesWritten( this )
    , readyRead( this )
    , remotefilesize_( 0 )
    , qeventloop_( new QEventLoop )
    , status_( NoReply )
{
    qnetworkreply_ = qnr;
    qnetworkreplyconn_ = new QNetworkReplyConn(qnetworkreply_, this);
    downloadProgress.notify( mCB(this,ODNetworkReply,downloaded) );
    error.notify( mCB(this,ODNetworkReply,errorOccurred) );
    finished.notify( mCB(this,ODNetworkReply,finish) );
    readyRead.notify( mCB(this,ODNetworkReply,readFromObj) );
    metaDataChanged.notify( mCB(this,ODNetworkReply,setRemoteFileSize) );
}


ODNetworkReply::~ODNetworkReply()
{ 
    delete qnetworkreply_;
    delete qnetworkreplyconn_;
    delete qeventloop_;
}


bool ODNetworkReply::downloaded( CallBacker* cb, od_int64 totalnr )
{
    remotefilesize_ = totalnr;
    return true;
}


bool ODNetworkReply::errorOccurred(CallBacker*)
{
    status_ = Error;
    if ( isEventLoopRunning() )
	stopEventLoop();
    return true;
}


bool ODNetworkReply::finish(CallBacker*)
{
    status_ = Finish;
    if ( isEventLoopRunning() )
	stopEventLoop();
    return true;
}


bool ODNetworkReply::readFromObj( CallBacker* )
{
    status_ = DataReady;
    if ( isEventLoopRunning() )
	stopEventLoop();
    return true;
}


bool ODNetworkReply::setRemoteFileSize( CallBacker* )
{
    remotefilesize_ = qnetworkreply_->header
			    ( QNetworkRequest::ContentLengthHeader ).toInt();
    return true;
}


void ODNetworkReply::startEventLoop()
{ qeventloop_->exec(); }


void ODNetworkReply::stopEventLoop()
{ qeventloop_->exit(); }


bool ODNetworkReply::isEventLoopRunning()
{ return qeventloop_->isRunning(); }

//void ODNetworkReply::setDestPath( const char* path )
//{ destpath_ = path; }
//
//
//void ODNetworkReply::setODNetworkTask(ODNetworkTask& bar)
//{ odnetworktask_ = &bar; }
