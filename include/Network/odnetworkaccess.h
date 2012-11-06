#ifndef odnetworkaccess_h
#define odnetworkaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Oct 2012
 RCS:		$Id: odnetworkaccess.h 26297 2012-09-21 00:16:36Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/


#include "bufstringset.h"
#include "callback.h"
#include "executor.h"

class BufferStringSet;
class QEventLoop;
class QNetworkAccessManager;
class QNAMConnector;
class ODNetworkReply;
class QNetworkReplyConn;


mClass(Network) ODNetworkAccess : public CallBacker
{
friend class QNAMConnector;

public:
				ODNetworkAccess();
				~ODNetworkAccess();

    enum State			{  };

    const char*			errorMsg(){ return errormsg_.buf(); }
    bool			finish(CallBacker*);

    bool			get(const char* url,const char* path);
    bool			getFiles(const BufferStringSet&,const char*,
					 TaskRunner* tr=0);

    void			startEventLoop();
    void			stopEventLoop();

    Notifier<ODNetworkAccess>	finished;

protected:

    BufferString		errormsg_;
    QEventLoop*			qeventloop_;
    QNetworkAccessManager*	qnam_;
    QNAMConnector*		qnamconn_;
    ODNetworkReply*		odnr_;
    
};


mClass(Network) ODNATask : public Executor
{
public:
				ODNATask(ODNetworkAccess* odna,
					 const BufferStringSet urls,
					 const BufferString dir)
				: Executor("Downloading Files")
				, odna_(odna)
				, urls_(urls)
				, outputdir_(dir)
				, nrdone_(0)		{}

    const char*			message() const;
    od_int64			nrDone() const;
    const char*			nrDoneText() const;
    od_int64			totalNr() const;

protected:

    int				nextStep();
    int				nrdone_;
    ODNetworkAccess*		odna_;
    const BufferStringSet	urls_;
    const BufferString		outputdir_;

};


#endif