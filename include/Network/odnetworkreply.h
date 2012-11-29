#ifndef odnetworkreply_h
#define odnetworkreply_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Oct 2012
 RCS:		$Id$
________________________________________________________________________

-*/


#include "networkmod.h"
#include "callback.h"

class QEventLoop;
class QNetworkReply;
class QNetworkReplyConn;


mClass(Network) ODNetworkReply : public CallBacker
{
    friend class QNetworkReplyConn;
    
public:

    enum Status			{ NoReply, DataReady, Finish, Error };

				ODNetworkReply(QNetworkReply*);
				~ODNetworkReply();

    od_int64			RemoteFileSize() const {return remotefilesize_;}

    int				status() const	    {return status_;}

    void			startEventLoop() const;
    void			stopEventLoop() const;
    bool			isEventLoopRunning() const;

    od_int64			remoteFileSize() const {return remotefilesize_;}


    Notifier<ODNetworkReply>	downloadProgress;
    Notifier<ODNetworkReply>	finished;
    Notifier<ODNetworkReply>	metaDataChanged;
    Notifier<ODNetworkReply>	error;
    Notifier<ODNetworkReply>	uploadProgress;
    Notifier<ODNetworkReply>	aboutToClose;
    Notifier<ODNetworkReply>	bytesWritten;
    Notifier<ODNetworkReply>	readyRead;

protected:

    bool			errorOccurred(CallBacker*);
    bool			finish(CallBacker*);
    bool			readFromObj(CallBacker*);
    bool			setRemoteFileSize(CallBacker*);

    od_int64			remotefilesize_;
    int				status_;
    QEventLoop*			qeventloop_;
    QNetworkReplyConn*		qnetworkreplyconn_;
    QNetworkReply*		qnetworkreply_;

};

#endif
