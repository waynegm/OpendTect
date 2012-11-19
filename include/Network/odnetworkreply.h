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
#include "bufstringset.h"
#include "strmprov.h"

class MyNetworkReply;
class QNetworkReply;
class QNetworkReplyConn;
class ODNetworkTask;


mClass(Network) ODNetworkReply : public CallBacker
{
    friend class QNetworkReplyConn;
    
public:
				ODNetworkReply(QNetworkReply*);
				~ODNetworkReply();

    const char*			errorMsg(){ return errormsg_.buf(); }

    bool			errorOccurred(CallBacker*);
    bool			finish(CallBacker*);
    bool			readFromObj(CallBacker*);

    void			setDestPath(const char*);
    void			setODNetworkTask(ODNetworkTask&);
    ODNetworkTask*		getODNetworkTask(){return odnetworktask_;}

    Notifier<ODNetworkReply>	downloadProgress;
    Notifier<ODNetworkReply>	finished;
    Notifier<ODNetworkReply>	metaDataChanged;
    Notifier<ODNetworkReply>	error;
    Notifier<ODNetworkReply>	uploadProgress;
    Notifier<ODNetworkReply>	aboutToClose;
    Notifier<ODNetworkReply>	bytesWritten;
    Notifier<ODNetworkReply>	readyRead;

protected:

    BufferString		errormsg_;
    BufferString		destpath_;
    StreamData			osd_;

    ODNetworkTask*		odnetworktask_;
    QNetworkReplyConn*		qnetworkreplyconn_;
    QNetworkReply*		qnetworkreply_;

};

#endif