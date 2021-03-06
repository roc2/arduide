/*
  win32/Serial.cpp

  This file is part of arduide, The Qt-based IDE for the open-source Arduino electronics prototyping platform.

  Copyright (C) 2010-2016 
  Authors : Denis Martinez
	    Martin Peres

This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * \file Serial.cpp
 * \author Denis Martinez
 * \date 2010-03-29
 */

#include "../Serial.h"

bool Serial::open(OpenMode mode)
{
    if (isOpen())
    {
        setErrorString(tr("Device (%0) already open").arg(mPort));
        return false;
    }

    DWORD dwBaudRate;
    switch (mBaudRate)
    {
    case 300: dwBaudRate = CBR_300; break;
    case 1200: dwBaudRate = CBR_1200; break;
    case 2400: dwBaudRate = CBR_2400; break;
    case 4800: dwBaudRate = CBR_4800; break;
    case 9600: dwBaudRate = CBR_9600; break;
    case 19200: dwBaudRate = CBR_19200; break;
    case 38400: dwBaudRate = CBR_38400; break;
    case 57600: dwBaudRate = CBR_57600; break;
    case 115200: dwBaudRate = CBR_115200; break;
    default:
        setErrorString(tr("Unknown baud rate %0").arg(mBaudRate));
        return false;
    }

    DWORD dwMode = 0;
    if (mode & ReadOnly)
        dwMode |= GENERIC_READ;
    if (mode & WriteOnly)
        dwMode |= GENERIC_READ;
    mSerial = ::CreateFile(mPort.toLocal8Bit().constData(),
                           dwMode, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    if (mSerial == INVALID_SERIAL_DESCRIPTOR)
        goto error;

    DCB dcb;
    FillMemory(&dcb, sizeof(dcb), 0);
    if (! GetCommState(mSerial, &dcb))
        goto error;
    dcb.BaudRate = dwBaudRate;
    if (! SetCommState(mSerial, &dcb))
        goto error;

    setOpenMode(mode);
    return true;

error:
    DWORD dwErr = ::GetLastError();
    LPSTR lpMsg;
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &lpMsg, 0, NULL);
    setErrorString(lpMsg);
    ::LocalFree(lpMsg);
    if (mSerial != INVALID_SERIAL_DESCRIPTOR)
    {
        ::CloseHandle(mSerial);
        mSerial = INVALID_SERIAL_DESCRIPTOR;
    }
    return false;
}

void Serial::close()
{
    if (isOpen())
    {
        emit aboutToClose();
        ::CloseHandle(mSerial);
        setOpenMode(NotOpen);
        setErrorString(QString());
        setInReadEventMode(false);
    }
}

qint64 Serial::readData(char *data, qint64 maxSize)
{
    DWORD dwRead;
    if (! ::ReadFile(mSerial, data, maxSize, &dwRead, NULL))
        goto error;
    return dwRead;

error:
    DWORD dwErr = GetLastError();
    LPSTR lpMsg;
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &lpMsg, 0, NULL);
    setErrorString(lpMsg);
    ::LocalFree(lpMsg);
    return -1;
}

qint64 Serial::writeData(const char *data, qint64 maxSize)
{
    DWORD dwWritten;
    if (! ::WriteFile(mSerial, data, maxSize, &dwWritten, NULL))
        goto error;
    return dwWritten;

error:
    DWORD dwErr = GetLastError();
    LPSTR lpMsg;
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &lpMsg, 0, NULL);
    setErrorString(lpMsg);
    ::LocalFree(lpMsg);
    return -1;
}

bool Serial::waitForReadyRead (int msecs)
{
    return false;
}

bool Serial::setDTR(bool enable)
{
    if (! isOpen())
        return false;
    return EscapeCommFunction(mSerial, enable ? SETDTR : CLRDTR);
}
