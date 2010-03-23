/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of duihome.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include "mainwindow.h"
#include "home.h"

#include <QGLWidget>
#include <DuiGLRenderer>
#include <DuiApplication>
#include <DuiSceneManager>
#include "x11wrapper.h"

MainWindow *MainWindow::mainWindowInstance = NULL;
QGLContext *MainWindow::openGLContext = NULL;

MainWindow::MainWindow(QWidget *parent) :
    DuiWindow(parent)
{
    mainWindowInstance = this;
    if (qgetenv("DUIHOME_DESKTOP") != "0") {
        // Dont Set the window type to desktop if DUIHOME_DESKTOP is set to 0
        setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
    }

    if (!DuiApplication::softwareRendering()) {
        // Get GL context
        QGLWidget *w = dynamic_cast<QGLWidget *>(viewport());
        if (w != NULL) {
            openGLContext = const_cast<QGLContext *>(w->context());
        }
    }

#ifdef Q_WS_X11
    // Visibility change messages are required to make the appVisible() signal work
    WId window = winId();
    XWindowAttributes attributes;
    Display *display = QX11Info::display();
    X11Wrapper::XGetWindowAttributes(display, window, &attributes);
    X11Wrapper::XSelectInput(display, window, attributes.your_event_mask | VisibilityChangeMask);
#endif

    // Create Home; the scene manager must be created before this
    home = new Home;
    sceneManager()->showWindowNow(home);

    excludeFromTaskBar();
}

MainWindow::~MainWindow()
{
    mainWindowInstance = NULL;
    openGLContext = NULL;
    delete home;
}

MainWindow *MainWindow::instance(bool create)
{
    if (mainWindowInstance == NULL && create) {
        // The static instance variable is set in the constructor
        new MainWindow;
    }

    return mainWindowInstance;
}

QGLContext *MainWindow::glContext()
{
    return openGLContext;
}

void MainWindow::excludeFromTaskBar()
{
    // Tell the window to not to be shown in the switcher
    Atom skipTaskbarAtom = X11Wrapper::XInternAtom(QX11Info::display(), "_NET_WM_STATE_SKIP_TASKBAR", False);
    changeNetWmState(true, skipTaskbarAtom);

    // Also set the _NET_WM_STATE window property to ensure Home doesn't try to
    // manage this window in case the window manager fails to set the property in time
    Atom netWmStateAtom = X11Wrapper::XInternAtom(QX11Info::display(), "_NET_WM_STATE", False);
    QVector<Atom> atoms;
    atoms.append(skipTaskbarAtom);
    X11Wrapper::XChangeProperty(QX11Info::display(), internalWinId(), netWmStateAtom, XA_ATOM, 32, PropModeReplace, (unsigned char *)atoms.data(), atoms.count());
}

void MainWindow::changeNetWmState(bool set, Atom one, Atom two)
{
    XEvent e;
    e.xclient.type = ClientMessage;
    Display *display = QX11Info::display();
    Atom netWmStateAtom = X11Wrapper::XInternAtom(display, "_NET_WM_STATE", FALSE);
    e.xclient.message_type = netWmStateAtom;
    e.xclient.display = display;
    e.xclient.window = internalWinId();
    e.xclient.format = 32;
    e.xclient.data.l[0] = set ? 1 : 0;
    e.xclient.data.l[1] = one;
    e.xclient.data.l[2] = two;
    e.xclient.data.l[3] = 0;
    e.xclient.data.l[4] = 0;
    X11Wrapper::XSendEvent(display, RootWindow(display, x11Info().screen()), FALSE, (SubstructureNotifyMask | SubstructureRedirectMask), &e);
    X11Wrapper::XSync(display, FALSE);
}
