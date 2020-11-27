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

#include <QxDao/QxSqlGenerator/QxSqlGenerator_Standard.h>

#include <QxDao/QxSqlDatabase.h>
#include <QxDao/IxDao_Helper.h>

#include <QxMemLeak/mem_leak.h>

namespace qx {
namespace dao {
namespace detail {

QxSqlGenerator_Standard::QxSqlGenerator_Standard() : IxSqlGenerator() {  }

QxSqlGenerator_Standard::~QxSqlGenerator_Standard() {  }

void QxSqlGenerator_Standard::init() {  }

QString QxSqlGenerator_Standard::getAutoIncrement() const { return QStringLiteral("AUTOINCREMENT"); }

QString QxSqlGenerator_Standard::getWildCard() const { return QStringLiteral("%"); }

QString QxSqlGenerator_Standard::getTableAliasSep() const { return QStringLiteral(" AS "); }

QString QxSqlGenerator_Standard::getLimit(const QxSqlLimit * pLimit) const
{
   if (! pLimit) { qAssert(false); return QStringLiteral(""); }
   QString sStartRow = pLimit->getStartRow_ParamKey();
   QString sRowsCount = pLimit->getRowsCount_ParamKey();
   return (QStringLiteral("LIMIT %1 OFFSET %2").arg(sRowsCount).arg(sStartRow));
}

void QxSqlGenerator_Standard::resolveLimit(QSqlQuery & query, const QxSqlLimit * pLimit) const
{
   if (! pLimit) { qAssert(false); return; }
   QString sStartRow = pLimit->getStartRow_ParamKey();
   QString sRowsCount = pLimit->getRowsCount_ParamKey();
   int iStartRow(pLimit->getStartRow()), iRowsCount(pLimit->getRowsCount());
   bool bQuestionMark = (qx::QxSqlDatabase::getSingleton()->getSqlPlaceHolderStyle() == qx::QxSqlDatabase::ph_style_question_mark);
   if (bQuestionMark) { query.addBindValue(iRowsCount); query.addBindValue(iStartRow); }
   else { query.bindValue(sRowsCount, iRowsCount); query.bindValue(sStartRow, iStartRow); }
}

void QxSqlGenerator_Standard::postProcess(QString & sql, const QxSqlLimit * pLimit) const { Q_UNUSED(sql); Q_UNUSED(pLimit); }

void QxSqlGenerator_Standard::onBeforeInsert(IxDao_Helper * pDaoHelper, void * pOwner) const { Q_UNUSED(pDaoHelper); Q_UNUSED(pOwner); }

void QxSqlGenerator_Standard::onAfterInsert(IxDao_Helper * pDaoHelper, void * pOwner) const { Q_UNUSED(pDaoHelper); Q_UNUSED(pOwner); }

void QxSqlGenerator_Standard::onBeforeUpdate(IxDao_Helper * pDaoHelper, void * pOwner) const { Q_UNUSED(pDaoHelper); Q_UNUSED(pOwner); }

void QxSqlGenerator_Standard::onAfterUpdate(IxDao_Helper * pDaoHelper, void * pOwner) const { Q_UNUSED(pDaoHelper); Q_UNUSED(pOwner); }

void QxSqlGenerator_Standard::onBeforeDelete(IxDao_Helper * pDaoHelper, void * pOwner) const { Q_UNUSED(pDaoHelper); Q_UNUSED(pOwner); }

void QxSqlGenerator_Standard::onAfterDelete(IxDao_Helper * pDaoHelper, void * pOwner) const { Q_UNUSED(pDaoHelper); Q_UNUSED(pOwner); }

void QxSqlGenerator_Standard::checkSqlInsert(IxDao_Helper * pDaoHelper, QString & sql) const { Q_UNUSED(pDaoHelper); Q_UNUSED(sql); }

void QxSqlGenerator_Standard::onBeforeSqlPrepare(IxDao_Helper * pDaoHelper, QString & sql) const { Q_UNUSED(pDaoHelper); Q_UNUSED(sql); }

void QxSqlGenerator_Standard::formatSqlQuery(IxDao_Helper * pDaoHelper, QString & sql) const
{
   Q_UNUSED(pDaoHelper);

   if (sql.startsWith(QStringLiteral("SELECT "))) { sql = QStringLiteral("\nSELECT ") + sql.right(sql.size() - 7); }
   else if (sql.startsWith(QStringLiteral("INSERT "))) { sql = QStringLiteral("\nINSERT ") + sql.right(sql.size() - 7); }
   else if (sql.startsWith(QStringLiteral("UPDATE "))) { sql = QStringLiteral("\nUPDATE ") + sql.right(sql.size() - 7); }
   else if (sql.startsWith(QStringLiteral("DELETE "))) { sql = QStringLiteral("\nDELETE ") + sql.right(sql.size() - 7); }
   else if (sql.startsWith(QStringLiteral("CREATE "))) { sql = QStringLiteral("\nCREATE ") + sql.right(sql.size() - 7); }

   sql.replace(QStringLiteral(" FROM "), QStringLiteral("\n  FROM "));
   sql.replace(QStringLiteral(" WHERE "), QStringLiteral("\n  WHERE "));
   sql.replace(QStringLiteral(" LEFT OUTER JOIN "), QStringLiteral("\n  LEFT OUTER JOIN "));
   sql.replace(QStringLiteral(" INNER JOIN "), QStringLiteral("\n  INNER JOIN "));
   sql.replace(QStringLiteral(" ORDER BY "), QStringLiteral("\n  ORDER BY "));
   sql.replace(QStringLiteral(" GROUP BY "), QStringLiteral("\n  GROUP BY "));
   sql.replace(QStringLiteral(" AND "), QStringLiteral("\n  AND "));
   sql.replace(QStringLiteral(" OR "), QStringLiteral("\n  OR "));
   sql.replace(QStringLiteral(" VALUES "), QStringLiteral("\n  VALUES "));
   sql.replace(QStringLiteral(" SET "), QStringLiteral("\n  SET "));
   sql.replace(QStringLiteral(" RETURNING "), QStringLiteral("\n  RETURNING "));
   sql += QStringLiteral("\n");
}

} // namespace detail
} // namespace dao
} // namespace qx
