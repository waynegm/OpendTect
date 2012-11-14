#ifndef i_qbutton_h
#define i_qbutton_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibutton.h"

#include <QAbstractButton>

/*! Help class, because templates can not use signals/slots
    Relays QT button signals to the notifyHandler of a uiButton object.
*/

QT_BEGIN_NAMESPACE

class i_ButMessenger : public QObject 
{ 
    Q_OBJECT
public:

i_ButMessenger( mQtclass(QAbstractButton)* sndr, uiButton* receiver )
    : receiver_(receiver)
    , sender_(sndr)
{
    connect( sender_, SIGNAL(clicked()), this, SLOT(clicked()) );
    connect( sender_, SIGNAL(toggled(bool)), this, SLOT(toggled(bool)) );
}

    mQtclass(QAbstractButton)* getSender() { return sender_; }
private:

    uiButton*			receiver_;
    mQtclass(QAbstractButton)*	sender_;

public slots:
    void toggled(bool yn)	{ receiver_->toggled( yn ); }
    void clicked()		{ receiver_->clicked(); }

};

QT_END_NAMESPACE

#endif
