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

#include <QtCore/qelapsedtimer.h>

#include <QxDao/IxDao_Helper.h>

#include <QxRegister/IxClass.h>

#include <QxMemLeak/mem_leak.h>

#define QX_DAO_ERR_INTERNAL                  QStringLiteral("'qx::dao' internal error")
#define QX_DAO_ERR_UNKNOWN_ERROR             QStringLiteral("'qx::dao' unknown error")
#define QX_DAO_ERR_NO_CONNECTION             QStringLiteral("error retrieving valid sql connection to database")
#define QX_DAO_ERR_OPEN_CONNECTION           QStringLiteral("unable to open connection to database")
#define QX_DAO_ERR_BUILD_SQL_QUERY           QStringLiteral("error building sql query : {}")
#define QX_DAO_ERR_EXECUTE_SQL_QUERY         QStringLiteral("execute sql query failed : {}")
#define QX_DAO_ERR_PREPARE_SQL_QUERY         QStringLiteral("prepare sql query failed : {}")
#define QX_DAO_ERR_NO_DATA                   QStringLiteral("sql query returns no data")
#define QX_DAO_ERR_NO_QUERY_BUILDER          QStringLiteral("unable to construct sql query builder")
#define QX_DAO_ERR_NO_ELEMENT_IN_CONTAINER   QStringLiteral("no element in container")
#define QX_DAO_ERR_INVALID_PRIMARY_KEY       QStringLiteral("invalid primary key")
#define QX_DAO_ERR_INVALID_SQL_RELATION      QStringLiteral("invalid sql relation")
#define QX_DAO_ERR_INVALID_VALUES_DETECTED   QStringLiteral("validator engine : invalid values detected")
#define QX_DAO_ERR_READ_ONLY                 QStringLiteral("cannot execute INSERT, UPDATE or DELETE query with a read only entity")

#define QX_CONSTRUCT_IX_DAO_HELPER() \
m_timeTotal(0), m_timeExec(0), m_timeNext(0), m_timePrepare(0), m_timeBuildHierarchy(0), m_timeBuildCppInstance(0), m_timeReadCppInstance(0), \
m_timeBuildSql(0), m_timeOpen(0), m_timeTransaction(0), m_nextCount(0), m_lDataCount(0), m_bTransaction(false), m_bQuiet(false), m_bTraceQuery(true), \
m_bTraceRecord(false), m_bCartesianProduct(false), m_bValidatorThrowable(false), m_bNeedToClearDatabaseByThread(false), \
m_bMongoDB(false), m_bDisplayTimerDetails(false), m_pDataMemberX(NULL), m_pDataId(NULL), m_pSqlGenerator(NULL)

#if (QT_VERSION >= 0x040800)
#define QX_DAO_TIMER_ELAPSED(timer) timer.nsecsElapsed()
#else // (QT_VERSION >= 0x040800)
#define QX_DAO_TIMER_ELAPSED(timer) (timer.elapsed() * 1000000) // convert milli-seconds to nano-seconds
#endif // (QT_VERSION >= 0x040800)

namespace qx
{
namespace dao
{
namespace detail
{

struct IxDao_Helper::IxDao_HelperImpl
{

    QElapsedTimer  m_timerTotal;                       //!< Timer to manage total elapsed time : build SQL query + all database operations + fetch C++ classes
    QElapsedTimer  m_timerExec;                        //!< Timer to manage database exec() elapsed time
    QElapsedTimer  m_timerNext;                        //!< Timer to manage database next() elapsed time
    QElapsedTimer  m_timerPrepare;                     //!< Timer to manage database prepare() elapsed time
    QElapsedTimer  m_timerBuildHierarchy;              //!< Timer to manage build relationships hierarchy elapsed time
    QElapsedTimer  m_timerBuildCppInstance;            //!< Timer to manage build C++ instance elapsed time
    QElapsedTimer  m_timerReadCppInstance;             //!< Timer to manage read C++ instance elapsed time
    QElapsedTimer  m_timerBuildSql;                    //!< Timer to manage build SQL query elapsed time
    QElapsedTimer  m_timerOpen;                        //!< Timer to manage open database elapsed time
    QElapsedTimer  m_timerTransaction;                 //!< Timer to manage database transaction (commit/rollback) elapsed time
    qint64         m_timeTotal;                        //!< Time (in nanoseconds) for total elapsed time : build SQL query + all database operations + fetch C++ classes
    qint64         m_timeExec;                         //!< Time (in nanoseconds) to execute SQL query in database
    qint64         m_timeNext;                         //!< Time (in nanoseconds) of all calls of QSqlQuery::next() method
    qint64         m_timePrepare;                      //!< Time (in nanoseconds) of all calls of QSqlQuery::prepare() method
    qint64         m_timeBuildHierarchy;               //!< Time (in nanoseconds) to build relationships hierarchy
    qint64         m_timeBuildCppInstance;             //!< Time (in nanoseconds) to build C++ instance
    qint64         m_timeReadCppInstance;              //!< Time (in nanoseconds) to read C++ instance
    qint64         m_timeBuildSql;                     //!< Time (in nanoseconds) to build SQL query
    qint64         m_timeOpen;                         //!< Time (in nanoseconds) to open database
    qint64         m_timeTransaction;                  //!< Time (in nanoseconds) for database transaction (commit/rollback)
    int            m_nextCount;                        //!< Number of calls of QSqlQuery::next() method
    QSqlDatabase   m_database;                         //!< Connection to database to execute query
    QSqlQuery      m_query;                            //!< Query to execute
    QSqlError      m_error;                            //!< Error executing query
    QString        m_context;                          //!< Description of context : fetch, insert, update, delete, etc...
    long           m_lDataCount;                       //!< Data member collection count
    bool           m_bTransaction;                     //!< Transaction in progress : commit if valid, rollback if error
    bool           m_bQuiet;                           //!< Display message and assert in debug mode
    bool           m_bTraceQuery;                      //!< Trace sql query
    bool           m_bTraceRecord;                     //!< Trace sql record
    bool           m_bCartesianProduct;                //!< Recordset can return cartesian product => same id in multiple records
    bool           m_bValidatorThrowable;              //!< An exception of type qx::validator_error is thrown when invalid values are detected inserting or updating an element into database
    QStringList    m_lstColumns;                       //!< List of columns to execute sql query (if empty => all columns)
    bool           m_bNeedToClearDatabaseByThread;     //!< Internal purpose only to clear current database context by thread in destructor
    bool           m_bMongoDB;                         //!< Current database context is a MongoDB database
    QStringList    m_lstItemsAsJson;                   //!< List of items to insert/update/delete as JSON (used for MongoDB database)
    bool           m_bDisplayTimerDetails;             //!< Display in logs all timers details (exec(), next(), prepare(), open(), etc...)

    qx::IxSqlQueryBuilder_ptr     m_pQueryBuilder;        //!< Sql query builder
    qx::IxDataMemberX* m_pDataMemberX;         //!< Collection of data member
    qx::IxDataMember* m_pDataId;              //!< Data member id
    qx::QxSqlQuery                m_qxQuery;              //!< Query sql with place-holder
    IxSqlGenerator* m_pSqlGenerator;        //!< SQL generator to build SQL query specific for each database
    qx::QxInvalidValueX           m_lstInvalidValues;     //!< List of invalid values using validator engine
    qx::QxSqlRelationLinked_ptr   m_pSqlRelationLinked;   //!< List of relation linked to build a hierarchy of relationships

    IxDao_HelperImpl(qx::IxSqlQueryBuilder* pBuilder, const qx::QxSqlQuery* pQuery) : QX_CONSTRUCT_IX_DAO_HELPER()
    {
        m_pQueryBuilder.reset(pBuilder); if (pQuery)
        {
            m_qxQuery = (*pQuery);
        }
    }
    ~IxDao_HelperImpl()
    {
        ;
    }

    void displaySqlQuery();

};

IxDao_Helper::IxDao_Helper(qx::IxSqlQueryBuilder* pBuilder, const qx::QxSqlQuery* pQuery /* = NULL */) : m_pImpl(new IxDao_HelperImpl(pBuilder, pQuery))
{
    ;
}

IxDao_Helper::~IxDao_Helper()
{
    terminate(); if (m_pImpl->m_bNeedToClearDatabaseByThread)
    {
        qx::QxSqlDatabase::getSingleton()->clearCurrentDatabaseByThread();
    }
}

bool IxDao_Helper::isValid() const
{
    return (!m_pImpl->m_error.isValid() && m_pImpl->m_pQueryBuilder);
}

bool IxDao_Helper::hasFeature(QSqlDriver::DriverFeature ft) const
{
    return (m_pImpl->m_database.driver() ? m_pImpl->m_database.driver()->hasFeature(ft) : false);
}

QSqlDatabase& IxDao_Helper::database()
{
    return m_pImpl->m_database;
}

const QSqlDatabase& IxDao_Helper::database() const
{
    return m_pImpl->m_database;
}

QSqlQuery& IxDao_Helper::query()
{
    return m_pImpl->m_query;
}

const QSqlQuery& IxDao_Helper::query() const
{
    return m_pImpl->m_query;
}

QSqlError& IxDao_Helper::error()
{
    return m_pImpl->m_error;
}

const QSqlError& IxDao_Helper::error() const
{
    return m_pImpl->m_error;
}

qx::QxSqlQuery& IxDao_Helper::qxQuery()
{
    return m_pImpl->m_qxQuery;
}

const qx::QxSqlQuery& IxDao_Helper::qxQuery() const
{
    return m_pImpl->m_qxQuery;
}

qx::IxDataMemberX* IxDao_Helper::getDataMemberX() const
{
    return m_pImpl->m_pDataMemberX;
}

long IxDao_Helper::getDataCount() const
{
    return m_pImpl->m_lDataCount;
}

qx::IxDataMember* IxDao_Helper::getDataId() const
{
    return m_pImpl->m_pDataId;
}

qx::IxDataMember* IxDao_Helper::nextData(long& l) const
{
    return (m_pImpl->m_pQueryBuilder ? m_pImpl->m_pQueryBuilder->nextData(l) : NULL);
}

QString IxDao_Helper::sql() const
{
    return (m_pImpl->m_pQueryBuilder ? m_pImpl->m_pQueryBuilder->getSqlQuery() : QStringLiteral(""));
}

qx::QxSqlRelationLinked* IxDao_Helper::getSqlRelationLinked() const
{
    return m_pImpl->m_pSqlRelationLinked.get();
}

bool IxDao_Helper::getCartesianProduct() const
{
    return m_pImpl->m_bCartesianProduct;
}

QStringList IxDao_Helper::getSqlColumns() const
{
    return m_pImpl->m_lstColumns;
}

void IxDao_Helper::setSqlColumns(const QStringList& lst)
{
    m_pImpl->m_lstColumns = lst;
}

IxSqlGenerator* IxDao_Helper::getSqlGenerator() const
{
    return m_pImpl->m_pSqlGenerator;
}

void IxDao_Helper::quiet()
{
    m_pImpl->m_bQuiet = true;
}

bool IxDao_Helper::isReadOnly() const
{
    return ((m_pImpl->m_pDataMemberX && m_pImpl->m_pDataMemberX->getClass()) ? m_pImpl->m_pDataMemberX->getClass()->isDaoReadOnly() : false);
}

bool IxDao_Helper::isMongoDB() const
{
    return m_pImpl->m_bMongoDB;
}

bool IxDao_Helper::getAddAutoIncrementIdToUpdateQuery() const
{
    return qx::QxSqlDatabase::getSingleton()->getAddAutoIncrementIdToUpdateQuery();
}

QStringList& IxDao_Helper::itemsAsJson()
{
    return m_pImpl->m_lstItemsAsJson;
}

bool IxDao_Helper::exec(bool bForceEmptyExec /* = false */)
{
    bool bExec = false;
    IxDao_Timer timer(this, IxDao_Helper::timer_db_exec);
    if ((m_pImpl->m_qxQuery.isEmpty()) && (!bForceEmptyExec))
    {
        bExec = this->query().exec(this->builder().getSqlQuery());
    }
    else
    {
        bExec = this->query().exec();
    }
    return bExec;
}

bool IxDao_Helper::prepare(QString& sql)
{
    QString sqlTemp = sql;
    IxDao_Timer timer(this, IxDao_Helper::timer_db_prepare);
    if (m_pImpl->m_pSqlGenerator)
    {
        m_pImpl->m_pSqlGenerator->onBeforeSqlPrepare(this, sql);
    }
    if (sqlTemp != sql)
    {
        xdebug("SQL query has been changed by SQL generator (onBeforeSqlPrepare) :\n   - before : '{}'\n   - after : '{}'", qPrintable(sqlTemp), qPrintable(sql));
    }
    return this->query().prepare(sql);
}

void IxDao_Helper::addInvalidValues(const qx::QxInvalidValueX& lst)
{
    m_pImpl->m_lstInvalidValues.insert(lst);
    if (m_pImpl->m_lstInvalidValues.count() > 0)
    {
        QString sInvalidValues = QX_DAO_ERR_INVALID_VALUES_DETECTED;
        sInvalidValues += QStringLiteral("\n") + m_pImpl->m_lstInvalidValues.text();
        updateError(sInvalidValues);
        if (m_pImpl->m_bValidatorThrowable)
        {
            QString tmp = m_pImpl->m_lstInvalidValues.text(); 
            xerror("invalid values detected, throw 'qx::validator_error' exception : '{}'", qPrintable(tmp));
        }
        if (m_pImpl->m_bValidatorThrowable)
        {
            throw qx::validator_error(m_pImpl->m_lstInvalidValues);
        }
    }
}

qx::IxSqlQueryBuilder& IxDao_Helper::builder()
{
    qAssert(m_pImpl->m_pQueryBuilder);
    return (*m_pImpl->m_pQueryBuilder);
}

const qx::IxSqlQueryBuilder& IxDao_Helper::builder() const
{
    qAssert(m_pImpl->m_pQueryBuilder);
    return (*m_pImpl->m_pQueryBuilder);
}

QSqlError IxDao_Helper::errFailed(bool bPrepare /* = false */)
{
    QString sql = this->sql();
    bool bFormatSql = qx::QxSqlDatabase::getSingleton()->getFormatSqlQueryBeforeLogging();
    if (bFormatSql && m_pImpl->m_pSqlGenerator)
    {
        m_pImpl->m_pSqlGenerator->formatSqlQuery(this, sql);
    }

    if (bPrepare)
    {
        xerror(qPrintable(QX_DAO_ERR_PREPARE_SQL_QUERY), qPrintable(sql));
    }
    else
    {
        xerror(qPrintable(QX_DAO_ERR_EXECUTE_SQL_QUERY), qPrintable(sql));
    }
    return updateError(m_pImpl->m_query.lastError());
}

QSqlError IxDao_Helper::errEmpty()
{
    QString sql = this->sql();
    xerror(qPrintable(QX_DAO_ERR_BUILD_SQL_QUERY), qPrintable(sql));
    return updateError(m_pImpl->m_query.lastError());
}

QSqlError IxDao_Helper::errNoData()
{
    return updateError(QX_DAO_ERR_NO_DATA);
}

QSqlError IxDao_Helper::errInvalidId()
{
    return updateError(QX_DAO_ERR_INVALID_PRIMARY_KEY);
}

QSqlError IxDao_Helper::errInvalidRelation()
{
    return updateError(QX_DAO_ERR_INVALID_SQL_RELATION);
}

QSqlError IxDao_Helper::errReadOnly()
{
    return updateError(QX_DAO_ERR_READ_ONLY);
}

bool IxDao_Helper::transaction()
{
    if (m_pImpl->m_bMongoDB)
    {
        return false;
    }
    if (isValid() && hasFeature(QSqlDriver::Transactions))
    {
        IxDao_Timer timer(this, IxDao_Helper::timer_db_transaction);
        m_pImpl->m_bTransaction = m_pImpl->m_database.transaction();
    }
    return m_pImpl->m_bTransaction;
}

bool IxDao_Helper::nextRecord()
{
    IxDao_Timer timer(this, IxDao_Helper::timer_db_next);
    bool bNext = m_pImpl->m_query.next();
    if (bNext && m_pImpl->m_bTraceRecord)
    {
        dumpRecord();
    }
    return bNext;
}

bool IxDao_Helper::updateSqlRelationX(const QStringList& relation)
{
    qx_bool bHierarchyOk(true);
    m_pImpl->m_bCartesianProduct = false;
    IxDao_Timer timer(this, IxDao_Helper::timer_cpp_build_hierarchy);
    m_pImpl->m_pSqlRelationLinked = qx::QxSqlRelationLinked::getHierarchy((m_pImpl->m_pDataMemberX ? m_pImpl->m_pDataMemberX->getClass() : NULL), relation, bHierarchyOk, this);
    if (!bHierarchyOk)
    {
        m_pImpl->m_pSqlRelationLinked.reset();
    }
    if (!bHierarchyOk)
    {
        QString txt = bHierarchyOk.getDesc();
        xdebug(qPrintable(txt));
        return false;
    }
    m_pImpl->m_bCartesianProduct = m_pImpl->m_pSqlRelationLinked->getCartesianProduct();
    if (m_pImpl->m_pQueryBuilder)
    {
        m_pImpl->m_pQueryBuilder->setCartesianProduct(m_pImpl->m_bCartesianProduct);
    }
    if (m_pImpl->m_pQueryBuilder)
    {
        m_pImpl->m_pQueryBuilder->setHashRelation(relation.join(QStringLiteral("|")));
    }
    if (m_pImpl->m_bCartesianProduct)
    {
        m_pImpl->m_pQueryBuilder->initIdX(m_pImpl->m_pSqlRelationLinked->getAllRelationCount());
    }
    return bHierarchyOk.getValue();
}

void IxDao_Helper::dumpRecord() const
{
    if (!m_pImpl->m_query.isValid())
    {
        return;
    }
    QString sDump; QVariant v;
    QSqlRecord record = m_pImpl->m_query.record();
    int iCount = record.count();
    if (iCount <= 0)
    {
        return;
    }
    for (int i = 0; i < iCount; ++i)
    {
        v = record.value(i); 
        sDump += (v.isNull() ? QStringLiteral("NULL") : v.toString()) + QStringLiteral("|");
    }
    sDump = sDump.left(sDump.count() - 1); // Remove last QStringLiteral("|")
    xdebug("dump sql record : {}", qPrintable(sDump));
}

void IxDao_Helper::timerStart(IxDao_Helper::timer_type timer)
{
    switch (timer)
    {
    case IxDao_Helper::timer_total:
        m_pImpl->m_timerTotal.start();
        break;
    case IxDao_Helper::timer_db_exec:
        m_pImpl->m_timerExec.start();
        break;
    case IxDao_Helper::timer_db_next:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        m_pImpl->m_timerNext.start();
        break;
    case IxDao_Helper::timer_db_prepare:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        m_pImpl->m_timerPrepare.start();
        break;
    case IxDao_Helper::timer_cpp_build_hierarchy:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        m_pImpl->m_timerBuildHierarchy.start();
        break;
    case IxDao_Helper::timer_cpp_build_instance:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        m_pImpl->m_timerBuildCppInstance.start();
        break;
    case IxDao_Helper::timer_cpp_read_instance:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        m_pImpl->m_timerReadCppInstance.start();
        break;
    case IxDao_Helper::timer_build_sql:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        m_pImpl->m_timerBuildSql.start();
        break;
    case IxDao_Helper::timer_db_open:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        m_pImpl->m_timerOpen.start();
        break;
    case IxDao_Helper::timer_db_transaction:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        m_pImpl->m_timerTransaction.start();
        break;
    default:
        break;
    }
}

qint64 IxDao_Helper::timerElapsed(IxDao_Helper::timer_type timer)
{
    qint64 elapsed = 0;
    switch (timer)
    {
    case IxDao_Helper::timer_total:
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerTotal);
        m_pImpl->m_timeTotal += elapsed;
        break;
    case IxDao_Helper::timer_db_exec:
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerExec);
        m_pImpl->m_timeExec += elapsed;
        break;
    case IxDao_Helper::timer_db_next:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerNext);
        m_pImpl->m_timeNext += elapsed;
        m_pImpl->m_nextCount++;
        break;
    case IxDao_Helper::timer_db_prepare:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerPrepare);
        m_pImpl->m_timePrepare += elapsed;
        break;
    case IxDao_Helper::timer_cpp_build_hierarchy:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerBuildHierarchy);
        m_pImpl->m_timeBuildHierarchy += elapsed;
        break;
    case IxDao_Helper::timer_cpp_build_instance:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerBuildCppInstance);
        m_pImpl->m_timeBuildCppInstance += elapsed;
        break;
    case IxDao_Helper::timer_cpp_read_instance:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerReadCppInstance);
        m_pImpl->m_timeReadCppInstance += elapsed;
        break;
    case IxDao_Helper::timer_build_sql:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerBuildSql);
        m_pImpl->m_timeBuildSql += elapsed;
        break;
    case IxDao_Helper::timer_db_open:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerOpen);
        m_pImpl->m_timeOpen += elapsed;
        break;
    case IxDao_Helper::timer_db_transaction:
        if (!m_pImpl->m_bDisplayTimerDetails)
        {
            break;
        }
        elapsed = QX_DAO_TIMER_ELAPSED(m_pImpl->m_timerTransaction);
        m_pImpl->m_timeTransaction += elapsed;
        break;
    default:
        break;
    }
    return elapsed;
}

void IxDao_Helper::addQuery(bool bResolve)
{
    if (m_pImpl->m_qxQuery.isEmpty())
    {
        return;
    }
    IxDao_Timer timer(this, IxDao_Helper::timer_build_sql);
    QString sql = this->builder().getSqlQuery();
    QString sqlToAdd = m_pImpl->m_qxQuery.query().trimmed();
    bool bAddSqlCondition = false;
    if (sqlToAdd.left(6).contains(QStringLiteral("WHERE "), Qt::CaseInsensitive))
    {
        sqlToAdd = sqlToAdd.right(sqlToAdd.size() - 6); bAddSqlCondition = true;
    }
    else if (sqlToAdd.left(4).contains(QStringLiteral("AND "), Qt::CaseInsensitive))
    {
        sqlToAdd = sqlToAdd.right(sqlToAdd.size() - 4); bAddSqlCondition = true;
    }
    this->builder().replaceSqlQueryAlias(sqlToAdd);
    sql += (bAddSqlCondition ? qx::IxSqlQueryBuilder::addSqlCondition(sql) : QStringLiteral(" ")) + sqlToAdd;
    if (m_pImpl->m_qxQuery.isDistinct() && sql.left(7).contains(QStringLiteral("SELECT "), Qt::CaseInsensitive))
    {
        sql = QStringLiteral("SELECT DISTINCT ") + sql.right(sql.size() - 7);
    }
    m_pImpl->m_qxQuery.postProcess(sql);
    this->builder().setSqlQuery(sql);

    if (bResolve)
    {
        if (!this->prepare(sql))
        {
            this->errFailed(true);
        }
        m_pImpl->m_qxQuery.resolve(this->query());
    }
}

QSqlError IxDao_Helper::updateError(const QString& sError)
{
    QString sDriverText = QStringLiteral("%1 <%2>").arg(QX_DAO_ERR_INTERNAL).arg(m_pImpl->m_context);
    sDriverText += (sql().isEmpty() ? QStringLiteral("") : (QStringLiteral(" : ") + sql()));
    QSqlError err = QSqlError(sDriverText, sError, QSqlError::UnknownError);
    return updateError(err);
}

QSqlError IxDao_Helper::updateError(const QSqlError& error)
{
    if (!m_pImpl->m_error.isValid())
    {
        m_pImpl->m_error = error;
        return m_pImpl->m_error;
    }

    QString databaseText = ((error.databaseText() != m_pImpl->m_error.databaseText()) && (!error.databaseText().isEmpty())) ? (m_pImpl->m_error.databaseText() + QStringLiteral("\n") + error.databaseText()) : m_pImpl->m_error.databaseText();
    QString driverText = ((error.driverText() != m_pImpl->m_error.driverText()) && (!error.driverText().isEmpty())) ? (m_pImpl->m_error.driverText() + QStringLiteral("\n") + error.driverText()) : m_pImpl->m_error.driverText();
    QSqlError::ErrorType type = (((m_pImpl->m_error.type() == QSqlError::NoError) || (m_pImpl->m_error.type() == QSqlError::UnknownError)) && (error.type() != QSqlError::NoError)) ? error.type() : m_pImpl->m_error.type();
    QString nativeErrorCode = (!m_pImpl->m_error.nativeErrorCode().isEmpty()) ? error.nativeErrorCode() : m_pImpl->m_error.nativeErrorCode();
    m_pImpl->m_error = QSqlError(driverText, databaseText, type, nativeErrorCode);

    return m_pImpl->m_error;
}

void IxDao_Helper::init(QSqlDatabase* pDatabase, const QString& sContext)
{
    timerStart(IxDao_Helper::timer_total);
    m_pImpl->m_context = sContext;
    m_pImpl->m_bTraceQuery = qx::QxSqlDatabase::getSingleton()->getTraceSqlQuery();
    m_pImpl->m_bTraceRecord = qx::QxSqlDatabase::getSingleton()->getTraceSqlRecord();
    m_pImpl->m_bMongoDB = (qx::QxSqlDatabase::getSingleton()->getDriverName() == QStringLiteral("QXMONGODB"));
    m_pImpl->m_bDisplayTimerDetails = qx::QxSqlDatabase::getSingleton()->getDisplayTimerDetails();
    qAssert(!m_pImpl->m_context.isEmpty());

#ifndef _QX_ENABLE_MONGODB
    if (m_pImpl->m_bMongoDB)
    {
        qAssertMsg(false, "_QX_ENABLE_MONGODB compilation option is required to connect to MongoDB database");
        updateError(QStringLiteral("QXMONGODB driver : _QX_ENABLE_MONGODB compilation option is required to connect to MongoDB database")); 
        return;
    }
#endif // _QX_ENABLE_MONGODB

    if (!m_pImpl->m_bMongoDB)
    {
        QSqlError dbError;
        IxDao_Timer timer(this, IxDao_Helper::timer_db_open);
        if (pDatabase)
        {
            m_pImpl->m_bNeedToClearDatabaseByThread = qx::QxSqlDatabase::getSingleton()->setCurrentDatabaseByThread(pDatabase);
        }
        m_pImpl->m_database = (pDatabase ? (*pDatabase) : qx::QxSqlDatabase::getDatabase(dbError));
        if (dbError.isValid())
        {
            updateError(dbError); return;
        }
        if (!m_pImpl->m_database.isValid())
        {
            updateError(QX_DAO_ERR_NO_CONNECTION); return;
        }
        if (!m_pImpl->m_database.isOpen() && !m_pImpl->m_database.open())
        {
            updateError(QX_DAO_ERR_OPEN_CONNECTION); return;
        }
        if (!m_pImpl->m_pQueryBuilder)
        {
            updateError(QX_DAO_ERR_NO_QUERY_BUILDER); return;
        }
        m_pImpl->m_query = QSqlQuery(m_pImpl->m_database);
        m_pImpl->m_query.setForwardOnly(true);
    }

    m_pImpl->m_pQueryBuilder->init();
    m_pImpl->m_pQueryBuilder->setDaoHelper(this);
    m_pImpl->m_pDataMemberX = (m_pImpl->m_pQueryBuilder ? m_pImpl->m_pQueryBuilder->getDataMemberX() : NULL);
    m_pImpl->m_lDataCount = (m_pImpl->m_pQueryBuilder ? m_pImpl->m_pQueryBuilder->getDataCount() : 0);
    m_pImpl->m_pDataId = (m_pImpl->m_pQueryBuilder ? m_pImpl->m_pQueryBuilder->getDataId() : NULL);
    m_pImpl->m_pSqlGenerator = qx::QxSqlDatabase::getSingleton()->getSqlGenerator();
    m_pImpl->m_bValidatorThrowable = qx::QxSqlDatabase::getSingleton()->getValidatorThrowable();
}

void IxDao_Helper::terminate()
{
    if ((m_pImpl->m_lstInvalidValues.count() > 0) && m_pImpl->m_bValidatorThrowable)
    {
        if (m_pImpl->m_bTransaction)
        {
            IxDao_Timer timer(this, IxDao_Helper::timer_db_transaction);
            m_pImpl->m_database.rollback();
        }
    }
    else if (!isValid())
    {
        if (m_pImpl->m_bTransaction)
        {
            IxDao_Timer timer(this, IxDao_Helper::timer_db_transaction);
            m_pImpl->m_database.rollback();
        }
        if (!m_pImpl->m_bQuiet)
        {
            QString ierr = m_pImpl->m_error.nativeErrorCode();
            QString tmp = m_pImpl->m_error.driverText();
            xerror("Database error number '{}' : {}", qPrintable(ierr), qPrintable(tmp));
            tmp = m_pImpl->m_error.databaseText(); xwarn(qPrintable(tmp));
            if (m_pImpl->m_bMongoDB)
            {
                tmp = m_pImpl->m_qxQuery.queryAt(0); xwarn(qPrintable(tmp));
            }
        }
    }
    else if (m_pImpl->m_pQueryBuilder)
    {
        if (m_pImpl->m_bTransaction)
        {
            IxDao_Timer timer(this, IxDao_Helper::timer_db_transaction);
            m_pImpl->m_database.commit();
        }
        if (!m_pImpl->m_bQuiet && m_pImpl->m_bTraceQuery)
        {
            timerElapsed(IxDao_Helper::timer_total);
            m_pImpl->displaySqlQuery();
        }
    }
    else
    {
        if (m_pImpl->m_bTransaction)
        {
            IxDao_Timer timer(this, IxDao_Helper::timer_db_transaction);
            m_pImpl->m_database.rollback();
        }
        if (!m_pImpl->m_bQuiet)
        {
            xerror(qPrintable(QX_DAO_ERR_UNKNOWN_ERROR));
            qAssert(false);
        }
    }

    m_pImpl->m_bTransaction = false;
    dumpBoundValues();
}

void IxDao_Helper::dumpBoundValues() const
{
    if (m_pImpl->m_bMongoDB)
    {
        return;
    }
    qx::QxSqlDatabase* pDatabase = qx::QxSqlDatabase::getSingleton(); if (!pDatabase)
    {
        return;
    }
    bool bBoundValues = pDatabase->getTraceSqlBoundValues();
    bool bBoundValuesOnError = pDatabase->getTraceSqlBoundValuesOnError();

    if ((!isValid() && bBoundValuesOnError) || (bBoundValues))
    {
        qx::QxSqlQuery::dumpBoundValues(m_pImpl->m_query);
    }
}

void IxDao_Helper::IxDao_HelperImpl::displaySqlQuery()
{
    QString query = (m_bMongoDB ? m_qxQuery.queryAt(0) : QString());
    QString sql = ((query.isEmpty() && m_pQueryBuilder) ? m_pQueryBuilder->getSqlQuery() : query);
    bool bFormatSql = qx::QxSqlDatabase::getSingleton()->getFormatSqlQueryBeforeLogging();
    qx::dao::detail::IxSqlGenerator* pSqlGenerator = qx::QxSqlDatabase::getSingleton()->getSqlGenerator();
    qint64 iTraceSqlOnlySlowQueriesDatabase = static_cast<qint64>(qx::QxSqlDatabase::getSingleton()->getTraceSqlOnlySlowQueriesDatabase());
    qint64 iTraceSqlOnlySlowQueriesTotal = static_cast<qint64>(qx::QxSqlDatabase::getSingleton()->getTraceSqlOnlySlowQueriesTotal());
    if ((iTraceSqlOnlySlowQueriesDatabase > 0) && (iTraceSqlOnlySlowQueriesTotal < 0))
    {
        iTraceSqlOnlySlowQueriesTotal = 999999999;
    }
    else if ((iTraceSqlOnlySlowQueriesTotal > 0) && (iTraceSqlOnlySlowQueriesDatabase < 0))
    {
        iTraceSqlOnlySlowQueriesDatabase = 999999999;
    }
    if (bFormatSql && pSqlGenerator)
    {
        pSqlGenerator->formatSqlQuery(NULL, sql);
    }

    if ((m_timeTotal >= (iTraceSqlOnlySlowQueriesTotal * 1000000)) || (m_timeExec >= (iTraceSqlOnlySlowQueriesDatabase * 1000000))) // convert milli-seconds to nano-seconds
    {
        QString log = QStringLiteral("sql query (total: %1 ms").arg((m_timeTotal == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeTotal) / 1000000.0), 'g', 3));
        log += QStringLiteral(", db_exec: %1 ms").arg((m_timeExec == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeExec) / 1000000.0), 'g', 3));
        if (m_bDisplayTimerDetails)
        {
            log += QStringLiteral(", db_prepare: %1 ms").arg((m_timePrepare == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timePrepare) / 1000000.0), 'g', 3));
            log += QStringLiteral(", db_next(%1): %2 ms").arg(m_nextCount).arg((m_timeNext == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeNext) / 1000000.0), 'g', 3));
            log += QStringLiteral(", db_open: %1 ms").arg((m_timeOpen == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeOpen) / 1000000.0), 'g', 3));
            log += QStringLiteral(", db_transaction: %1 ms").arg((m_timeTransaction == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeTransaction) / 1000000.0), 'g', 3));
            log += QStringLiteral(", build_relations: %1 ms").arg((m_timeBuildHierarchy == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeBuildHierarchy) / 1000000.0), 'g', 3));
            log += QStringLiteral(", build_sql: %1 ms").arg((m_timeBuildSql == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeBuildSql) / 1000000.0), 'g', 3));
            log += QStringLiteral(", build_cpp: %1 ms").arg((m_timeBuildCppInstance == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeBuildCppInstance) / 1000000.0), 'g', 3));
            log += QStringLiteral(", read_cpp: %1 ms").arg((m_timeReadCppInstance == 0) ? QStringLiteral("0") : QString::number((static_cast<double>(m_timeReadCppInstance) / 1000000.0), 'g', 3));
        }
        log += QStringLiteral(") : ") + sql;
        xdebug(qPrintable(log));
    }
}

} // namespace detail
} // namespace dao
} // namespace qx
