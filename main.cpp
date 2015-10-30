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

#include <iostream>

#include <QSettings>
#include <QApplication>
#include <QSurfaceFormat>
#include <QNetworkInterface>
#include <QCommandLineParser>

#include <gst/gst.h>

#include "window.h"

static QString mac() {
    for (const QNetworkInterface& netInterface : QNetworkInterface::allInterfaces()) {
        // Return only the first non-loopback, non-point-to-point MAC Address
        if (!(netInterface.flags() & (QNetworkInterface::IsLoopBack | QNetworkInterface::IsPointToPoint)))
            return netInterface.hardwareAddress();
    }

    return QString();
}

static std::string red     = "\x1b[31;1m";
static std::string green   = "\x1b[32;1m";
static std::string yellow  = "\x1b[33;1m";
static std::string magenta = "\x1b[35;1m";
static std::string cyan    = "\x1b[36;1m";
static std::string restore = "\x1b[0m\n";

int main(int argc, char *argv[]) {
    gst_init (NULL, NULL);
    QApplication::setApplicationName("disupurei");
    QApplication::setApplicationVersion("0.1");
    QApplication::setOrganizationName("Carbonium Development");
    QApplication::setOrganizationDomain("https://github.com/jgilje/disupurei");
    QApplication app(argc, argv);
    QSettings settings;

    std::cout << yellow << std::endl <<
    "    ____  _                                  _   " << std::endl <<
    "   / __ \\(_)______  ______  __  __________  (_) " << std::endl <<
    "  / / / / / ___/ / / / __ \\/ / / / ___/ _ \\/ / " << std::endl <<
    " / /_/ / (__  ) /_/ / /_/ / /_/ / /  /  __/ /    " << std::endl <<
    "/_____/_/____/\\__,_/ .___/\\__,_/_/   \\___/_/  " << std::endl <<
    "                  /_/                            " << std::endl <<
    std::endl <<
    green << "     2015 - Joakim L. Gilje\n" <<
    "            " << qPrintable(QApplication::organizationDomain()) << "\n" <<
    restore << std::endl << std::flush;

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    QCommandLineParser parser;
    QCommandLineOption configOption = QCommandLineOption({{"c", "config"}, "View and set configuration"});
    QCommandLineOption urlOption = QCommandLineOption({{"u", "url"}, "Set server URL", "url"});
    QCommandLineOption macOption = QCommandLineOption({{"m", "mac"}, "Set/Override mac address", "mac"});
    QCommandLineOption windowOption = QCommandLineOption({{"w", "window"}, "Start in windowed mode"});
    parser.setApplicationDescription("disupurei - info screen client");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(configOption);
    parser.addOption(urlOption);
    parser.addOption(macOption);
    parser.addOption(windowOption);
    parser.process(app);

    if (parser.isSet(configOption)) {
        if (parser.isSet(urlOption)) {
            settings.setValue("url", parser.value(urlOption));
        }

        if (parser.isSet(macOption)) {
            settings.setValue("mac", parser.value(macOption));
        }

        std::cout << cyan << "Configuration" << std::endl;
        if (settings.contains("url")) {
            std::cout << "\tURL: " << magenta << qPrintable(settings.value("url").toString()) << std::endl;
        } else {
            std::cout << "\tURL: " << red << "Not set" << std::endl;
        }

        std::cout << cyan;
        if (settings.contains("mac")) {
            std::cout << "\tMAC: " << magenta << qPrintable(settings.value("url").toString()) << std::endl;
        } else {
            std::cout << "\tMAC: " << magenta << qPrintable(mac()) << " (detected)" << std::endl;
        }

        std::cout << restore;
        return 0;
    }

    QString macAddress = settings.value("mac", mac()).toString();
    QString url = settings.value("url").toString();

    if (macAddress.isEmpty()) {
        if (settings.contains("mac")) {
            std::cout << red << "Mac address set to empty in config. Set to any value, or remove entry to use host mac address."
                      << restore << std::endl;
            return 1;
        } else {
            std::cout << red << "No mac address detected. Use --config --mac <mac> to set a static mac address."
                      << restore << std::endl;
            return 1;
        }
    }

    if (url.isEmpty()) {
        std::cout << red << "No url set. Use --config --url <url> to set a url."
                  << restore << std::endl;
        return 1;
    }

    DisupureiWindow window;
    window.playlist().macAddress(macAddress);
    window.playlist().url(url);

    if (parser.isSet(windowOption)) {
        window.show();
    } else {
        window.showFullScreen();
    }

    return app.exec();
}
