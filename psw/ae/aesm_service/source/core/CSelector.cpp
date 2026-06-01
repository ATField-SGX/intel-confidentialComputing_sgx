/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "CSelector.h"
#include "ICommunicationSocket.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

CSelector::CSelector(IServerSocket* serverSock) :
    m_serverSock(serverSock),
    m_epollFd(-1),
    m_serverFd(-1),
    m_terminationFd(-1),
    m_eventCount(0),
    m_canAcceptConnection(false)
{
    m_connectedSockets.clear();

    m_epollFd = epoll_create(1);
    if (m_epollFd < 0) {
        throw "Epoll creation failed";
    }
}


CSelector::~CSelector()
{
    if (m_epollFd >= 0) {
        close(m_epollFd);
        m_epollFd = -1;
    }
}

void CSelector::addSocket(ICommunicationSocket* socket)
{
    if (socket == NULL) {
        return;
    }

    addFd(socket->getSockDescriptor());
    m_connectedSockets.push_back(socket);
}

void CSelector::removeSocket(ICommunicationSocket* socket)
{
    if (socket == NULL) {
        return;
    }

    removeFd(socket->getSockDescriptor());
    m_connectedSockets.remove(socket);
}

bool CSelector::select(int fd_term)
{
    registerServerSocket();
    registerTerminationFd(fd_term);

    m_canAcceptConnection = false;

    size_t monitoredFdCount = m_connectedSockets.size();
    if (m_serverFd != -1) {
        monitoredFdCount++;
    }
    if (m_terminationFd != -1) {
        monitoredFdCount++;
    }

    if (monitoredFdCount == 0) {
        throw "No file descriptors registered for epoll wait";
    }

    m_events.resize(monitoredFdCount);

    do {
        m_eventCount = epoll_wait(m_epollFd, &m_events.front(), static_cast<int>(m_events.size()), -1);
    } while (m_eventCount == -1 && errno == EINTR);

    if (m_eventCount < 0) {
        throw "Epoll wait failed";
    }

    for (int i = 0; i < m_eventCount; i++) {
        int fd = m_events[i].data.fd;
        uint32_t events = m_events[i].events;
        if (fd_term != -1 && fd == fd_term) {
            return false;
        }
        if (fd == m_serverFd) {
            if ((events & (EPOLLERR | EPOLLHUP)) != 0) {
                throw "Epoll reported error on server socket";
            }
            if ((events & EPOLLIN) != 0) {
                m_canAcceptConnection = true;
            }
        }
    }

    return true;
}

bool CSelector::canAcceptConnection()
{
    return m_canAcceptConnection;
}

std::list<ICommunicationSocket*> CSelector::getSocsWithNewContent()
{
    std::list<ICommunicationSocket*> socketswithContent;

    for (int i = 0; i < m_eventCount; i++) {
        int fd = m_events[i].data.fd;
        if (fd == m_serverFd || fd == m_terminationFd) {
            continue;
        }

        ICommunicationSocket* socket = detachSocket(fd);
        if (socket != NULL) {
            socketswithContent.push_back(socket);
        }
    }

    return socketswithContent;
}

void CSelector::registerServerSocket()
{
    int serverFd = m_serverSock->getSockDescriptor();

    if (serverFd < 0) {
        throw "Invalid server file descriptor";
    }

    if (serverFd == m_serverFd) {
        return;
    }

    if (m_serverFd != -1) {
        removeFd(m_serverFd);
    }

    addFd(serverFd);
    m_serverFd = serverFd;
}

void CSelector::registerTerminationFd(int fd_term)
{
    if (fd_term == m_terminationFd) {
        return;
    }

    if (m_terminationFd != -1) {
        removeFd(m_terminationFd);
    }

    if (fd_term != -1) {
        addFd(fd_term);
    }

    m_terminationFd = fd_term;
}

void CSelector::addFd(int fd)
{
    if (fd < 0) {
        throw "Invalid file descriptor";
    }

    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = EPOLLIN;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) != 0) {
        throw "Epoll add failed";
    }
}

void CSelector::removeFd(int fd)
{
    if (fd == -1) {
        return;
    }

    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, NULL) != 0 && errno != ENOENT && errno != EBADF) {
        throw "Epoll delete failed";
    }
}

ICommunicationSocket* CSelector::detachSocket(int fd)
{
    std::list<ICommunicationSocket*>::iterator it = m_connectedSockets.begin();

    while (it != m_connectedSockets.end()) {
        ICommunicationSocket* socket = *it;
        if (socket->getSockDescriptor() == fd) {
            removeFd(fd);
            m_connectedSockets.erase(it);
            return socket;
        }
        ++it;
    }

    return NULL;
}
