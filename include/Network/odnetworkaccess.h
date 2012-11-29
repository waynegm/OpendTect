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

    const char*			errorMsg() const    { return errormsg_.buf(); }

    bool			getFile(const char* url,const char* path,
					TaskRunner* tr=0) const;
    bool			getFiles(const BufferStringSet&,const char*,
					 TaskRunner* tr=0) const;

    void			setHttpProxy(const char* hostname,int port,bool 
					     auth=false,const char* username=0,
					     const char* password=0);

    void			startEventLoop() const;
    void			stopEventLoop() const;
    bool			isEventLoopRunning() const;

    Notifier<ODNetworkAccess>	finished;

protected:

    mutable BufferString	errormsg_;

    QEventLoop*			qeventloop_;
    QNetworkAccessManager*	qnam_;
    QNAMConnector*		qnamconn_;

};

mGlobal(Network) ODNetworkAccess& ODNA();

#endif
