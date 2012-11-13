#ifndef i_qobjectptr_h
#define i_qobjectptr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2012
 RCS:           $Id: i_qtreeview.h 26341 2012-09-23 13:30:12Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "callback.h"

#include <QObject>



//!brief Weak pointer to a QObject

QT_BEGIN_NAMESPACE

class i_QObjectPtr : public QObject, public CallBacker
{
    Q_OBJECT
public:
			
				i_QObjectPtr( QObject* sndr  = 0 )
				    : sender_( 0 )
				    , destroyed( this )
				{ 
				    set( sndr );
				}
    
    Notifier<i_QObjectPtr>	destroyed;
    
	virtual			~i_QObjectPtr() { set( 0 ); }

    QObject*			get() { return sender_; }
    const QObject*		get() const { return sender_; }
    
				operator bool () { return sender_; }
    QObject*			operator->() { return sender_; }
    const QObject*		operator->() const { return sender_; }

    QObject*			operator=(QObject* sndr) { return set( sndr ); }
    bool			operator==(const QObject* o){return o==sender_;}
    bool			operator!=(const QObject* o){return o!=sender_;}

    QObject*			set( QObject* sndr )
				{
				    if ( sender_ )
				    {
					disconnect(sender_,SIGNAL(destroyed()),
						   this,SLOT(itemDestroyed()));
				    }
				    
				    sender_ = sndr;
				    
				    if ( sender_ )
				    {
					connect( sndr, SIGNAL(destroyed()),
						 this, SLOT(itemDestroyed()) );
				    }
				    
				    return sndr;
				}

private:

    QObject* 		sender_;

private slots:

    void itemDestroyed()
    {
	destroyed.trigger();
	sender_ = 0;
    }

};

QT_END_NAMESPACE

#endif
