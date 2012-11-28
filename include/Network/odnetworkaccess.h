#ifndef odnetworkaccess_h
#define odnetworkaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Oct 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "callback.h"
#include "executor.h"
#include "networkmod.h"
#include "strmprov.h"

class QEventLoop;
class QNetworkAccessManager;
class QNAMConnector;
class ODNetworkReply;
class QNetworkReply;


mClass(Network) ODNetworkAccess : public CallBacker
{
friend class QNAMConnector;

public:
				ODNetworkAccess();
				~ODNetworkAccess();

    enum State			{  };

    const char*			errorMsg(){ return errormsg_.buf(); }
    bool			finish(CallBacker*);

    bool			getFile(const char* url,const char* path,
				    TaskRunner* tr=0);
    bool			getFiles(const BufferStringSet&,const char*,
					 TaskRunner* tr=0);
    void			setHttpProxy(const char* hostname,int port,bool 
					     auth=false,const char* username=0,
					     const char* password=0);

    void			startEventLoop();
    void			stopEventLoop();
    bool			isEventLoopRunning();

    Notifier<ODNetworkAccess>	finished;

protected:

    BufferString		errormsg_;

    QEventLoop*			qeventloop_;
    QNetworkAccessManager*	qnam_;
    QNAMConnector*		qnamconn_;
    ODNetworkReply*		odnr_;

    const BufferStringSet	urls_;
    const BufferString		outputdir_;
    
};


mClass(Network) ODNetworkTask : public Executor
{
public:

				ODNetworkTask(QNetworkAccessManager* qnam, 
					       const char* url,const char* dest)
				: Executor("Downloading Files")
				, qnam_(qnam)
				, outputdest_(dest)
				, url_(url)
				, nrdone_(0)
				, totalnr_(0)
				, init_(true)
				, msg_(0)	    {}


    const char*			message() const;
    od_int64			nrDone() const;
    const char*			nrDoneText() const;
    od_int64			totalNr() const;

    void			setTotalNr(const od_int64);

    bool			readData();
    bool			errorOccured();

protected:

    int				nextStep();
    od_int64			nrdone_;
    od_int64			totalnr_;
    BufferString		msg_;

    bool			init_;
    StreamData			osd_;

    ODNetworkReply*		odnr_;
    QNetworkAccessManager*	qnam_;
    QNetworkReply*		qnetworkreply_;

    const BufferString		url_;
    const BufferString		outputdest_;
};

#endif
