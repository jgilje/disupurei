/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "window.h"

#include <QtWidgets>
#include <QTimer>

DisupureiWindow::DisupureiWindow() {
    _layout.addWidget(&_videoPlayer);
    _layout.addWidget(&_imagePlayer);
    setLayout(&_layout);

    setWindowTitle(tr("disupurei"));
    _timer.setSingleShot(true);

    connect(&_timer, &QTimer::timeout, this, &DisupureiWindow::onEntryFinished);
    connect(&_videoPlayer, &VideoPlayer::finished, this, &DisupureiWindow::onEntryFinished);
    connect(&_imagePlayer, &ImagePlayer::timeout, this, &DisupureiWindow::onEntryFinished);
    connect(&_playlist, &Playlist::playlistAvailable, this, &DisupureiWindow::onPlaylistAvailable);

    // the Gst Pipeline needs to be initialized after we have a window opened
    connect(this, &DisupureiWindow::windowOpened, &_videoPlayer, &VideoPlayer::initPipeline, Qt::QueuedConnection);
    connect(this, &DisupureiWindow::windowOpened, this, [this] {
        _playlist.checkForCachedMetadata();
    }, Qt::QueuedConnection);
}

Playlist &DisupureiWindow::playlist() {
    return _playlist;
}

void DisupureiWindow::keyReleaseEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Escape:
        QApplication::quit();
        break;
    case Qt::Key_F:
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        break;
    default:
        QOpenGLWidget::keyReleaseEvent(event);
    }
}

void DisupureiWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    emit windowOpened();
}

void DisupureiWindow::onEntryFinished() {
    const Entry& entry = _playlist.next();
    qDebug() << "Playing back" << entry.fileId << "(" << entry.type << ")";

    switch (entry.type) {
    case Playlist::Type::IMAGE:
        _layout.setCurrentWidget(&_imagePlayer);
        _imagePlayer.open(entry.filePath, entry.durationMillis);
        break;
    case Playlist::Type::VIDEO:
        _layout.setCurrentWidget(&_videoPlayer);
        _videoPlayer.open(entry.filePath);
        break;
    }
}

void DisupureiWindow::onPlaylistAvailable() {
    qDebug() << Q_FUNC_INFO;

    _videoPlayer.stop();
    _imagePlayer.stop();

    onEntryFinished();
}
