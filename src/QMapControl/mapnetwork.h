/***************************************************************************
 *   Copyright (C) 2007 by Kai Winter   *
 *   kaiwinter@gmx.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef MAPNETWORK_H
#define MAPNETWORK_H

#include <QObject>
#include <QDebug>
#include <QList>
#include <QQueue>
#include <QPixmap>
#include <QMutex>
#include <QUrl>

#include "IImageManager.h"
/**
    @author Kai Winter <kaiwinter@gmx.de>
*/
class ImageManager;
class MapNetwork : QObject
{
    Q_OBJECT
    public:
        MapNetwork(IImageManager* parent);
        ~MapNetwork();

        void load(const QString& hash, const QString& host, const QString& url);

        /*!
         * checks if the given url is already loading
         * @param url the url of the image
         * @return boolean, if the image is already loading
         */
        bool isLoading(QString hash);

        /*!
         * Aborts all current loading threads.
         * This is useful when changing the zoom-factor, though newly needed images loads faster
        */
        void abortLoading();

    private:
        IImageManager* parent;
        QNetworkAccessManager* m_networkManager;
        QMap<QNetworkReply*, LoadingRequest*> loadingMap;
        QMap<QTimer*, QNetworkReply*> timeoutMap;
        QQueue<LoadingRequest*> loadingRequests;

        MapNetwork& operator=(const MapNetwork& rhs);
        MapNetwork(const MapNetwork& old);
        void launchRequest();
        void launchRequest(QUrl url, LoadingRequest* R);


    private slots:
        void requestFinished(QNetworkReply* reply);
        void timeout();
};

#endif
