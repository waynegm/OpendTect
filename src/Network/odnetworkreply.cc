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

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>


ODNetworkReply::ODNetworkReply(QNetworkReply* qnr)
    : downloadProgress( this )
    , finished( this )
    , metaDataChanged( this )
    , error( this )
    , uploadProgress( this )
    , aboutToClose( this )
    , bytesWritten( this )
    , readyRead( this )
{
    qnetworkreply_ = qnr;
    qnetworkreplyconn_ = new QNetworkReplyConn(qnetworkreply_, this);
    error.notify( mCB(this,ODNetworkReply,errorOccurred) );
    finished.notify( mCB(this,ODNetworkReply,finish) );
    readyRead.notify( mCB(this,ODNetworkReply,readFromObj) );
}


ODNetworkReply::~ODNetworkReply()
{ delete qnetworkreply_; }


bool ODNetworkReply::errorOccurred(CallBacker*)
{
    if( qnetworkreply_->error() > 0 )
    {
	errormsg_ = qPrintable( qnetworkreply_->errorString() );
	return true;
    }

    return false;
}


bool ODNetworkReply::finish(CallBacker*)
{
    osd_.close();
    return true;
}


bool ODNetworkReply::readFromObj( CallBacker* )
{
    od_int64 bytes = qnetworkreply_->bytesAvailable();
    char* buffer = new char[bytes];
    qnetworkreply_->read( buffer, bytes );
    FilePath fp = destpath_.buf();
    if ( !File::exists(fp.pathOnly()) )
	File::createDir( fp.pathOnly() );

    if ( !osd_.usable() )
    {
	osd_ = StreamProvider( destpath_ ).makeOStream();
	if ( !osd_.usable() )
	    return false;
    }

    osd_.ostrm->write( buffer, bytes );
    return true;
}


void ODNetworkReply::setDestPath( const char* path )
{ destpath_ = path; }


void ODNetworkReply::setODNetworkTask(ODNetworkTask& bar)
{ odnetworktask_ = &bar; }
