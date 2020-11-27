/****************************************************************************
**
** https://www.qxorm.com/
** Copyright (C) 2013 Lionel Marty (contact@qxorm.com)
**
** This file is part of the QxOrm library
**
** This software is provided 'as-is', without any express or implied
** warranty. In no event will the authors be held liable for any
** damages arising from the use of this software
**
** Commercial Usage
** Licensees holding valid commercial QxOrm licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Lionel Marty
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file 'license.gpl3.txt' included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met : http://www.gnu.org/copyleft/gpl.html
**
** If you are unsure which license is appropriate for your use, or
** if you have questions regarding the use of this file, please contact :
** contact@qxorm.com
**
****************************************************************************/

#include <QxPrecompiled.h>

#include <QxDao/QxSoftDelete.h>

#include <QxMemLeak/mem_leak.h>

namespace qx
{

QxSoftDelete::QxSoftDelete() : m_eMode(QxSoftDelete::mode_date_time), m_bFetchInJoin(true)
{}

QxSoftDelete::QxSoftDelete(const QString& sColumn) : m_sColumn(sColumn), m_eMode(QxSoftDelete::mode_date_time), m_bFetchInJoin(true)
{}

QxSoftDelete::QxSoftDelete(const QString& sColumn, QxSoftDelete::mode eMode) : m_sColumn(sColumn), m_eMode(eMode), m_bFetchInJoin(true)
{}

QxSoftDelete::~QxSoftDelete()
{}

QString QxSoftDelete::getTableName() const
{
    return m_sTable;
}

QString QxSoftDelete::getColumnName() const
{
    return m_sColumn;
}

QString QxSoftDelete::getSqlQueryToFetch() const
{
    return m_sSqlQueryToFetch;
}

QString QxSoftDelete::getSqlQueryToUpdate() const
{
    return m_sSqlQueryToUpdate;
}

QString QxSoftDelete::getSqlQueryToCreateTable() const
{
    return m_sSqlQueryToCreateTable;
}

QxSoftDelete::mode QxSoftDelete::getMode() const
{
    return m_eMode;
}

bool QxSoftDelete::getSqlFetchInJoin() const
{
    return m_bFetchInJoin;
}

void QxSoftDelete::setTableName(const QString& sTable)
{
    m_sTable = sTable;
}

void QxSoftDelete::setColumnName(const QString& sColumn)
{
    m_sColumn = sColumn;
}

void QxSoftDelete::setSqlQueryToFetch(const QString& s)
{
    m_sSqlQueryToFetch = s;
}

void QxSoftDelete::setSqlQueryToUpdate(const QString& s)
{
    m_sSqlQueryToUpdate = s;
}

void QxSoftDelete::setSqlQueryToCreateTable(const QString& s)
{
    m_sSqlQueryToCreateTable = s;
}

void QxSoftDelete::setMode(QxSoftDelete::mode eMode)
{
    m_eMode = eMode;
}

void QxSoftDelete::setSqlFetchInJoin(bool b)
{
    m_bFetchInJoin = b;
}

bool QxSoftDelete::isEmpty() const
{
    return (m_sTable.isEmpty() || m_sColumn.isEmpty());
}

QString QxSoftDelete::buildSqlTablePointName(const QString& sTable /* = QString() */) const
{
    if (this->isEmpty())
    {
        return QStringLiteral("");
    }
    QString sCurrTable = (sTable.isEmpty() ? m_sTable : sTable);
    sCurrTable.replace(QStringLiteral("."), QStringLiteral("_"));
    return QStringLiteral("%1.%2").arg(sCurrTable).arg(m_sColumn);
}

QString QxSoftDelete::buildSqlQueryToFetch(const QString& sTable /* = QString() */) const
{
    QString sCurrTable = (sTable.isEmpty() ? m_sTable : sTable);
    sCurrTable.replace(QStringLiteral("."), QStringLiteral("_"));
    if (this->isEmpty())
    {
        return QStringLiteral("");
    }
    else if (!m_sSqlQueryToFetch.isEmpty())
    {
        return m_sSqlQueryToFetch;
    }
    else if (m_eMode == QxSoftDelete::mode_flag)
    {
        return (QStringLiteral("(%1.%2 IS NULL OR %3.%4 = '' OR %5.%6 = '0')").arg(sCurrTable).arg(m_sColumn).arg(sCurrTable).arg(m_sColumn).arg(sCurrTable).arg(m_sColumn));
    }
    else if (m_eMode == QxSoftDelete::mode_date_time)
    {
        return (QStringLiteral("(%1.%2 IS NULL OR %3.%4 = '')").arg(sCurrTable).arg(m_sColumn).arg(sCurrTable).arg(m_sColumn));
    }
    qAssert(false); return QStringLiteral("");
}

QString QxSoftDelete::buildSqlQueryToUpdate() const
{
    if (this->isEmpty())
    {
        return QStringLiteral("");
    }
    else if (!m_sSqlQueryToUpdate.isEmpty())
    {
        return m_sSqlQueryToUpdate;
    }
    else if (m_eMode == QxSoftDelete::mode_flag)
    {
        return (m_sColumn + QStringLiteral(" = '1'"));
    }
    else if (m_eMode == QxSoftDelete::mode_date_time)
    {
        return QStringLiteral("%1 = '%2'").arg(m_sColumn).arg(QDateTime::currentDateTime().toString(QX_DAO_SOFT_DELETE_QDATETIME_FORMAT));
    }
    qAssert(false);
    return QStringLiteral("");
}

QString QxSoftDelete::buildSqlQueryToCreateTable() const
{
    if (this->isEmpty())
    {
        return QStringLiteral("");
    }
    else if (!m_sSqlQueryToCreateTable.isEmpty())
    {
        return m_sSqlQueryToCreateTable;
    }
    else if (m_eMode == QxSoftDelete::mode_flag)
    {
        return QStringLiteral("%1 TEXT").arg(m_sColumn);
    }
    else if (m_eMode == QxSoftDelete::mode_date_time)
    {
        return QStringLiteral("%1 TEXT").arg(m_sColumn);
    }
    qAssert(false); 
    return QStringLiteral("");
}

} // namespace qx
