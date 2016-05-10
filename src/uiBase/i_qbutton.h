#ifndef i_qbutton_h
#define i_qbutton_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/

#include "uibutton.h"

#include <QAbstractButton>
#include <QObject>

/*! Help class, because templates can not use signals/slots
    Relays QT button signals to the notifyHandler of a uiButton object.
*/

QT_BEGIN_NAMESPACE

class i_ButMessenger : public QObject
{

    Q_OBJECT
    friend class	uiButton;

public:

i_ButMessenger( QAbstractButton* sndr, uiButton* receiver )
    : receiver_(receiver)
    , sender_(sndr)
{
#define mConnectButMsngr(nm,args) \
    connect( sender_, SIGNAL(nm(args)), this, SLOT(nm(args)) )

    mConnectButMsngr( toggled, bool );
    mConnectButMsngr( clicked, );
    mConnectButMsngr( pressed, );
    mConnectButMsngr( released, );
}

private:

    uiButton*		receiver_;
    QAbstractButton*	sender_;

public slots:

    void toggled(bool)	{ receiver_->toggled(); }
    void clicked()	{ receiver_->clicked(); }
    void pressed()	{ receiver_->pressed(); }
    void released()	{ receiver_->released();}

};

QT_END_NAMESPACE

#endif
