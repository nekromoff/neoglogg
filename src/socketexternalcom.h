/*
 * Copyright (c) 2026+ Daniel Duris, dusoft@staznosti.sk
 * Copyright (c) 2009–2018 Nicolas Bonnefon and other contributors
 *
 * This file is part of neoglogg.
 *
 * neoglogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * neoglogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with neoglogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOCKETEXTERNALCOM_H
#define SOCKETEXTERNALCOM_H

#include "externalcom.h"

#include <QLocalServer>
#include <QSharedMemory>

class SocketExternalInstance : public ExternalInstance
{
public:
    SocketExternalInstance();

    void loadFile( const QString& file_name ) const override;
    uint32_t getVersion() const override;
private:
    QSharedMemory* memory_;
};

class SocketExternalCommunicator : public ExternalCommunicator
{
    Q_OBJECT
public:
    SocketExternalCommunicator();
    ~SocketExternalCommunicator();

    ExternalInstance* otherInstance() const override;
    void startListening() override;

public slots:
    qint32 version() const override;

private slots:
    void onConnection();

private:
     QSharedMemory* memory_;
     QLocalServer* server_;
};

#endif // SOCKETEXTERNALCOM_H
