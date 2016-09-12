/*
  Qt port of KJob and KCompositeJob classes

  Copyright (C) 2015 Tomáš Hotovec : Pure Qt => No dependency on KDE library

  Copyright (C) 2006 Kevin Ottens <ervin@kde.org>
  Copyright (C) 2000 Stephan Kulow <coolo@kde.org>
                     David Faure <faure@kde.org>


  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB. If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "jobinterface.h"
#include <QDebug>

/* *** QJob *** */
QJob::QJob(QObject *parent) :
    QObject(parent),
    m_error(0),
    m_autoDelete(true),
    m_finished(false),
    m_percentage(-1)
{
    ;
}


QJob::~QJob()
{
    if (!m_finished) {
        emit finished(this);
    }
}


bool QJob::kill(QJob::KillVerbosity verbosity)
{
    if (doKill())
    {
        setError(KilledJobError);

        if (verbosity!=Quietly)
        {
            emitResult();
        }
        else
        {
            // If we are displaying a progress dialog, remove it first.
            m_finished = true;
            emit finished(this);

            if (isAutoDelete())
                deleteLater();
        }

        return true;
    }
    else
    {
        return false;
    }
}


bool QJob::doKill()
{
    return false;
}

void QJob::emitResult()
{
     m_finished = true;
     // If we are displaying a progress dialog, remove it first.
     emit finished( this );

     emit result( this );

     if ( isAutoDelete() )
         deleteLater();
}

QJob::Capabilities QJob::capabilities() const
{
    return m_capabilities;
}

int QJob::error() const
{
    return m_error;
}

QString QJob::errorText() const
{
    return m_errorText;
}

QString QJob::errorString() const
{
    return errorText();
}

unsigned long QJob::percent() const
{
    return m_percentage;
}

bool QJob::isAutoDelete() const
{
    return m_autoDelete;
}

void QJob::setAutoDelete(bool autoDelete)
{
    m_autoDelete = autoDelete;
}

void QJob::setError( int errorCode )
{
    m_error = errorCode;
}

void QJob::setErrorText( const QString &errorText )
{
    m_errorText = errorText;
}

void QJob::setPercent(unsigned long percentage)
{
    if ( m_percentage!=percentage )
    {
        m_percentage = percentage;
        emit percent( this, percentage );
    }
}

void QJob::setCapabilities(Capabilities capabilities)
{
  m_capabilities = capabilities;
}

/* *** QMulti Job *** */
QMultiJob::QMultiJob(QObject *parent) :
    QJob(parent)
{
}

QMultiJob::~QMultiJob()
{
}


bool QMultiJob::addSubjob(QJob *job)
{
    if ( job == 0 || m_subjobs.contains( job ) )
    {
        return false;
    }

    job->setParent(this);
    m_subjobs.append(job);
    connect( job, SIGNAL(result(QJob*)), SLOT(onSubjobResult(QJob*)) );

    // Forward information from that subjob.
    connect( job, SIGNAL(infoMessage(QJob*,QString,QString)), SLOT(onInfoMessage(QJob*,QString,QString)) );

    return true;
}


bool QMultiJob::removeSubjob( QJob *job )
{
    if ( job == 0 )
    {
        return false;
    }

    job->setParent(0);
    m_subjobs.removeAll( job );

    return true;
}

bool QMultiJob::hasSubjobs()
{
    return !m_subjobs.isEmpty();
}

const QList<QJob*> &QMultiJob::subjobs() const
{
    return m_subjobs;
}

void QMultiJob::clearSubjobs()
{
    Q_FOREACH(QJob *job, m_subjobs) {
        job->setParent(0);
    }
    m_subjobs.clear();
}

void QMultiJob::onSubjobResult( QJob *job )
{
    // Did job have an error ?
    if ( job->error() && !error() )
    {
        // Store it in the parent only if first error
        setError( job->error() );
        setErrorText( job->errorText() );
        emitResult();
    }

    removeSubjob(job);
}

void QMultiJob::onInfoMessage( QJob *job, const QString &plain, const QString &rich )
{
    emit infoMessage( job, plain, rich );
}
