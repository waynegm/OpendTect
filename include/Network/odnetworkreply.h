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

    enum ReplyStatus		{ NoReply, DataReady, Finish, Error };


				ODNetworkReply(QNetworkReply*);
				~ODNetworkReply();


    bool			downloaded(CallBacker*,od_int64);
    bool			errorOccurred(CallBacker*);
    bool			finish(CallBacker*);
    bool			readFromObj(CallBacker*);
    bool			setRemoteFileSize(CallBacker*);
    od_int64			RemoteFileSize(){return remotefilesize_;}

    int				replyStatus(){return status_;}

    void			startEventLoop();
    void			stopEventLoop();
    bool			isEventLoopRunning();

    od_int64			remoteFileSize(){return remotefilesize_;}


    CNotifier<ODNetworkReply,od_int64>	downloadProgress;
    Notifier<ODNetworkReply>	finished;
    Notifier<ODNetworkReply>	metaDataChanged;
    Notifier<ODNetworkReply>	error;
    Notifier<ODNetworkReply>	uploadProgress;
    Notifier<ODNetworkReply>	aboutToClose;
    Notifier<ODNetworkReply>	bytesWritten;
    Notifier<ODNetworkReply>	readyRead;

protected:

    od_int64			remotefilesize_;
    int				status_;
    QEventLoop*			qeventloop_;
    QNetworkReplyConn*		qnetworkreplyconn_;
    QNetworkReply*		qnetworkreply_;

};

#endif
