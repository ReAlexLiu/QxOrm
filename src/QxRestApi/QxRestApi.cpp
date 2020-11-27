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

#include <QxRestApi/QxRestApi.h>

#include <QxDao/IxPersistable.h>
#include <QxDao/IxPersistableCollection.h>
#include <QxDao/IxPersistableList.h>
#include <QxDao/QxSqlError.h>
#include <QxDao/QxSqlQuery.h>
#include <QxDao/QxSqlDatabase.h>
#include <QxDao/QxSqlSaveMode.h>

#include <QxFactory/QxFactoryX.h>

#include <QxRegister/IxClass.h>
#include <QxRegister/QxClassX.h>

#include <QxValidator/QxInvalidValueX.h>

#ifndef _QX_NO_JSON
#include <QxSerialize/QJson/QxSerializeQJson_QxSqlQuery.h>
#include <QxSerialize/QJson/QxSerializeQJson_QStringList.h>
#include <QxSerialize/QJson/QxSerializeQJson_IxSqlElement.h>
#include <QxSerialize/QJson/QxSerializeQJson_QxCollection.h>
#include <QxSerialize/QJson/QxSerializeQJson_QxInvalidValue.h>
#include <QxSerialize/QJson/QxSerializeQJson_QxInvalidValueX.h>
#endif // _QX_NO_JSON

#include <QxCommon/QxException.h>
#include <QxCommon/QxExceptionCode.h>

#include <QxMemLeak/mem_leak.h>

namespace qx
{

struct QxRestApi::QxRestApiImpl
{

    QString m_requestId;                            //!< Request identifier (GUID for example)
    QString m_entity;                               //!< Entity where to process request
    QString m_action;                               //!< Action to execute (count, fetch_by_id, fetch_all, fetch_by_query, insert, update, etc...)
    QString m_function;                             //!< Entity static function to call with action 'call_entity_function' and signature : static QJsonValue myEntity::myFct(const QJsonValue & request)
    QStringList m_columns;                          //!< Columns to fetch
    QStringList m_relations;                        //!< Relations to fetch
    QStringList m_outputFormat;                     //!< JSON output format (same syntax as relations)
    QString m_database;                             //!< Database to use to process request
    QString m_query;                                //!< SQL query to execute
    QString m_data;                                 //!< Data used to process request
    QSqlError m_error;                              //!< Error after executing the request
    qx::IxPersistable_ptr m_instance;               //!< Current instance to execute request
    QSqlDatabase m_db;                              //!< Current database to execute request
    qx_query m_qxQuery;                             //!< Query used by some actions
    long m_countResult;                             //!< Result after a count query
    qx_bool m_existResult;                          //!< Result after a exist query
    QxInvalidValueX m_validateResult;               //!< Result after a validate query
    qx::dao::save_mode::e_save_mode m_eSaveMode;    //!< Save mode for 'save' action

#ifndef _QX_NO_JSON

    QJsonValue m_requestJson;              //!< Request which contains all parameters (as JSON format)
    QJsonValue m_responseJson;             //!< Response after executing the request (as JSON format)
    QJsonValue m_dataJson;                 //!< Data used to process request (as JSON format)
    QJsonValue m_errorJson;                //!< Error after executing the request (as JSON format)

#endif // _QX_NO_JSON

    QxRestApiImpl() : m_countResult(0), m_eSaveMode(qx::dao::save_mode::e_none)
    {
        ;
    }
    ~QxRestApiImpl()
    {
        ;
    }

#ifndef _QX_NO_JSON

    void clear();
    void resetRequest();
    QJsonValue processRequestAsArray(const QJsonValue& request);
    void buildError(int errCode, const QString& errDesc);
    void buildError(const QSqlError& error);
    bool parseRequest(const QString& request);
    bool createInstance();
    bool decodeRequest();
    bool checkRequest();
    bool executeAction();
    bool buildResponse();
    bool formatResponse();
    bool doRequest();
    void getMetaData();
    QJsonValue getMetaData(IxClass* pClass);
    qx_bool callEntityFunction();
    void getDatabases();

#endif // _QX_NO_JSON

};

QxRestApi::QxRestApi(QObject* parent /* = NULL */) : QObject(parent), m_pImpl(new QxRestApiImpl())
{
    ;
}

QxRestApi::~QxRestApi()
{
    ;
}

void QxRestApi::clearAll()
{
    (*m_pImpl) = QxRestApiImpl();
}

QString QxRestApi::getErrorDesc() const
{
    return (m_pImpl->m_error.isValid() ? (QStringLiteral("%1\n%2").arg(m_pImpl->m_error.driverText()).arg(m_pImpl->m_error.databaseText())) : QString());
}

QString QxRestApi::getErrorCode() const
{
    return (m_pImpl->m_error.isValid() ? m_pImpl->m_error.nativeErrorCode() : QStringLiteral("0"));
}

QSqlError QxRestApi::getError() const
{
    return m_pImpl->m_error;
}

void QxRestApi::setEntity(const QString& entity)
{
    m_pImpl->m_entity = entity;
}

void QxRestApi::setAction(const QString& action)
{
    m_pImpl->m_action = action;
}

void QxRestApi::setFunction(const QString& fct)
{
    m_pImpl->m_function = fct;
}

void QxRestApi::setColumns(const QStringList& columns)
{
    m_pImpl->m_columns = columns;
}

void QxRestApi::setRelations(const QStringList& relations)
{
    m_pImpl->m_relations = relations;
}

void QxRestApi::setOutputFormat(const QStringList& outputFormat)
{
    m_pImpl->m_outputFormat = outputFormat;
}

void QxRestApi::setDatabase(const QString& database)
{
    m_pImpl->m_database = database;
}

void QxRestApi::setQuery(const QString& query)
{
    m_pImpl->m_query = query;
}

void QxRestApi::setData(const QString& data)
{
    m_pImpl->m_data = data;
}

QString QxRestApi::processRequest(const QString& request)
{
#ifdef _QX_NO_JSON
    QString msg = QStringLiteral("QxOrm library must be built without _QX_NO_JSON compilation option to be able to call REST API module");
    m_pImpl->m_error = QSqlError(msg, QStringLiteral("Cannot process request with _QX_NO_JSON compilation option"), QSqlError::UnknownError);
    return QStringLiteral("{ \"error\": \"%1\" }").arg(msg);
#else // _QX_NO_JSON
    m_pImpl->clear();
    if (m_pImpl->parseRequest(request))
    {
        processRequest(m_pImpl->m_requestJson);
    }
    QJsonValue result = (m_pImpl->m_errorJson.isNull() ? m_pImpl->m_responseJson : m_pImpl->m_errorJson);
    return qx::cvt::to_string(result);
#endif // _QX_NO_JSON
}

#ifndef _QX_NO_JSON

void QxRestApi::setData(const QJsonValue& data)
{
    m_pImpl->m_dataJson = data;
}

QJsonValue QxRestApi::processRequest(const QJsonValue& request)
{
    m_pImpl->clear();
    m_pImpl->m_db = qx::QxSqlDatabase::getDatabase(m_pImpl->m_error);
    if (m_pImpl->m_error.isValid())
    {
        m_pImpl->buildError(m_pImpl->m_error); return m_pImpl->m_errorJson;
    }
    if (request.isArray())
    {
        return m_pImpl->processRequestAsArray(request);
    }
    m_pImpl->m_requestJson = request;
    if (!m_pImpl->doRequest())
    {
        return m_pImpl->m_errorJson;
    }
    return m_pImpl->m_responseJson;
}

QJsonValue QxRestApi::QxRestApiImpl::processRequestAsArray(const QJsonValue& request)
{
    QJsonArray responseArray;
    QJsonArray requestArray = request.toArray();
    if (requestArray.count() <= 0)
    {
        buildError(9999, QStringLiteral("Request array is empty")); return m_errorJson;
    }

    bool bTransaction = false;
    if (m_db.driver() && m_db.driver()->hasFeature(QSqlDriver::Transactions))
    {
        bTransaction = m_db.transaction();
    }

    for (int i = 0; i < requestArray.count(); i++)
    {
        resetRequest();
        m_requestJson = requestArray.at(i);
        if (!doRequest())
        {
            break;
        }
        responseArray.append(m_responseJson);
    }

    bool bTransactionOk = true;
    if (!m_errorJson.isNull() && bTransaction)
    {
        bTransactionOk = m_db.rollback();
    }
    else if (bTransaction)
    {
        bTransactionOk = m_db.commit();
    }
    if (m_errorJson.isNull() && !bTransactionOk)
    {
        buildError(m_db.lastError());
    }

    if (!m_errorJson.isNull())
    {
        return m_errorJson;
    }
    m_responseJson = responseArray;
    return m_responseJson;
}

bool QxRestApi::QxRestApiImpl::doRequest()
{
    if (!decodeRequest())
    {
        return false;
    }
    if (!checkRequest())
    {
        return false;
    }
    if (!createInstance())
    {
        return false;
    }
    if (!executeAction())
    {
        return false;
    }
    if (!buildResponse())
    {
        return false;
    }
    if (!formatResponse())
    {
        return false;
    }
    return true;
}

void QxRestApi::QxRestApiImpl::clear()
{
    // Reset error and instance
    m_responseJson = QJsonValue();
    m_errorJson = QJsonValue();
    m_error = QSqlError();
    m_instance.reset();
    m_db = QSqlDatabase();
}

void QxRestApi::QxRestApiImpl::resetRequest()
{
    m_requestId = QStringLiteral("");
    m_action = QStringLiteral("");
    m_entity = QStringLiteral("");
    m_data = QStringLiteral("");
    m_query = QStringLiteral("");
    m_function = QStringLiteral("");
    m_database = QStringLiteral("");
    m_qxQuery = qx_query();
    m_dataJson = QJsonValue();
    m_requestJson = QJsonValue();
    m_responseJson = QJsonValue();
    m_eSaveMode = qx::dao::save_mode::e_none;
    m_columns.clear();
    m_relations.clear();
    m_outputFormat.clear();
    m_instance.reset();
}

void QxRestApi::QxRestApiImpl::buildError(int errCode, const QString& errDesc)
{
    m_error = QSqlError(errDesc, QStringLiteral(""), QSqlError::UnknownError, QString::number(errCode));
    QJsonObject errJson; QJsonObject errDetail;
    errDetail.insert(QStringLiteral("code"), errCode);
    errDetail.insert(QStringLiteral("desc"), errDesc);
    errJson.insert(QStringLiteral("error"), errDetail);
    if (!m_requestId.isEmpty())
    {
        errJson.insert(QStringLiteral("request_id"), m_requestId);
    }
    m_errorJson = errJson;
}

void QxRestApi::QxRestApiImpl::buildError(const QSqlError& error)
{
    if (!error.isValid())
    {
        return;
    }
    m_error = error;
    QJsonObject errJson; QJsonObject errDetail;
    errDetail.insert(QStringLiteral("code"), error.nativeErrorCode());
    errDetail.insert(QStringLiteral("desc"), (QStringLiteral("%1\n%2").arg(error.driverText()).arg(error.databaseText())));
    errJson.insert(QStringLiteral("error"), errDetail);
    if (!m_requestId.isEmpty())
    {
        errJson.insert(QStringLiteral("request_id"), m_requestId);
    }
    m_errorJson = errJson;
}

bool QxRestApi::QxRestApiImpl::createInstance()
{
    // Some actions doesn't require any instance
    if (m_action == QStringLiteral("get_meta_data"))
    {
        return true;
    }
    if (m_action == QStringLiteral("get_databases"))
    {
        return true;
    }
    if (m_action == QStringLiteral("call_custom_query"))
    {
        return true;
    }
    if (m_action == QStringLiteral("call_entity_function"))
    {
        return true;
    }

    // Check if entity implements qx::IxPersistable interface
    m_instance.reset();
    if (m_entity.isEmpty())
    {
        buildError(9999, QStringLiteral("JSON request is invalid : 'entity' field is empty"));
        return false;
    }
    if (!qx::QxClassX::implementIxPersistable(m_entity))
    {
        buildError(9999, QStringLiteral("Entity '%1' must implement qx::IxPersistable interface").arg(m_entity));
        return false;
    }
    m_instance = qx::IxPersistable_ptr(static_cast<qx::IxPersistable*>(qx::create_void_ptr(m_entity)));
    if (!m_instance)
    {
        buildError(9999, QStringLiteral("Entity '%1' is not valid : unable to create a new qx::IxPersistable instance").arg(m_entity));
        return false;
    }

    // Check if action requires some input data
    if (m_action == QStringLiteral("count"))
    {
        return true;
    }
    if (m_action == QStringLiteral("delete_all"))
    {
        return true;
    }
    if (m_action == QStringLiteral("destroy_all"))
    {
        return true;
    }
    if (m_action == QStringLiteral("delete_by_query"))
    {
        return true;
    }
    if (m_action == QStringLiteral("destroy_by_query"))
    {
        return true;
    }

    // Check if some input data has been provided by caller
    if (m_dataJson.isNull() && (m_action != QStringLiteral("fetch_all")) && (m_action != QStringLiteral("fetch_by_query")))
    {
        buildError(9999, QStringLiteral("No data provided for entity '%1'").arg(m_entity));
        return false;
    }

    // Check data format : single instance, or array of instances, or collection of instances (hash-map key/value)
    if (m_dataJson.isArray())
    {
        std::shared_ptr<qx::IxPersistableCollection> lst;
        QJsonArray dataArray = m_dataJson.toArray(); if (dataArray.count() <= 0)
        {
            buildError(9999, QStringLiteral("Data array is empty for entity '%1'").arg(m_entity));
            return false;
        }
        QJsonValue dataFirst = dataArray.first(); if (dataFirst.isNull())
        {
            buildError(9999, QStringLiteral("First item in data array is null for entity '%1'").arg(m_entity));
            return false;
        }
        if (!dataFirst.isObject())
        {
            buildError(9999, QStringLiteral("First item in data array is not an object for entity '%1'").arg(m_entity));
            return false;
        }
        QJsonObject dataFirstObject = dataFirst.toObject(); if (dataFirstObject.isEmpty())
        {
            buildError(9999, QStringLiteral("First object in data array is empty for entity '%1'").arg(m_entity));
            return false;
        }
        if ((dataFirstObject.count() == 2) && dataFirstObject.contains(QStringLiteral("key")) && dataFirstObject.contains(QStringLiteral("value")))
        {
            lst = m_instance->qxNewPersistableCollection(false);
        }
        else
        {
            lst = m_instance->qxNewPersistableCollection(true);
        }
        m_instance = std::static_pointer_cast<qx::IxPersistable>(lst);
    }
    else if ((m_action == QStringLiteral("fetch_all")) || (m_action == QStringLiteral("fetch_by_query")))
    {
        std::shared_ptr<qx::IxPersistableCollection> lst;
        lst = m_instance->qxNewPersistableCollection(true);
        m_instance = std::static_pointer_cast<qx::IxPersistable>(lst);
    }

    // Fill data in qx::IxPersistable instance
    qx_bool bFromJson = (m_dataJson.isNull() ? qx_bool(true) : m_instance->fromJson_(m_dataJson));
    if (!bFromJson)
    {
        buildError(static_cast<int>(bFromJson.getCode()), bFromJson.getDesc()); return false;
    }
    return true;
}

bool QxRestApi::QxRestApiImpl::parseRequest(const QString& request)
{
    // Parse request as JSON format
    QJsonParseError jsonError;
    QByteArray requestAsByteArray = request.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(requestAsByteArray, (&jsonError));
    if (jsonError.error != QJsonParseError::NoError)
    {
        buildError(static_cast<int>(jsonError.error), QStringLiteral("Error parsing JSON request : %1").arg(jsonError.errorString())); return false;
    }

    // Request can be an array or a single object
    m_requestJson = (doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object()));
    return true;
}

bool QxRestApi::QxRestApiImpl::decodeRequest()
{
    // Check if request is a valid JSON object
    if (m_requestJson.isNull())
    {
        buildError(9999, QStringLiteral("Request is NULL")); return false;
    }
    if (!m_requestJson.isObject())
    {
        buildError(9999, QStringLiteral("Request is not a JSON object")); return false;
    }
    QJsonObject request = m_requestJson.toObject();

    // Extract request identifier
    if (request.contains(QStringLiteral("request_id")))
    {
        m_requestId = request.value(QStringLiteral("request_id")).toString();
    }

    // Extract action
    if (request.contains(QStringLiteral("action")))
    {
        m_action = request.value(QStringLiteral("action")).toString();
    }
    else
    {
        buildError(9999, QStringLiteral("Parameter 'action' is required and cannot be empty")); return false;
    }
    if (m_action.isEmpty())
    {
        buildError(9999, QStringLiteral("Parameter 'action' cannot be empty")); return false;
    }

    // Extract entity
    if (request.contains(QStringLiteral("entity")))
    {
        m_entity = request.value(QStringLiteral("entity")).toString();
    }

    // Extract data
    if (request.contains(QStringLiteral("data")))
    {
        m_dataJson = request.value(QStringLiteral("data"));
    }
    else if (!m_data.isEmpty())
    {
        qx_bool bParseOk = qx::cvt::from_string(m_data, m_dataJson);
        if (!bParseOk)
        {
            buildError(static_cast<int>(bParseOk.getCode()), bParseOk.getDesc()); return false;
        }
    }

    // Extract function
    if (request.contains(QStringLiteral("fct")))
    {
        qx::cvt::from_json(request.value(QStringLiteral("fct")), m_function);
    }

    // Extract columns
    if (request.contains(QStringLiteral("columns")))
    {
        qx::cvt::from_json(request.value(QStringLiteral("columns")), m_columns);
    }

    // Extract relations
    if (request.contains(QStringLiteral("relations")))
    {
        qx::cvt::from_json(request.value(QStringLiteral("relations")), m_relations);
    }

    // Extract output format
    if (request.contains(QStringLiteral("output_format")))
    {
        qx::cvt::from_json(request.value(QStringLiteral("output_format")), m_outputFormat);
    }

    // Extract database key
    if (request.contains(QStringLiteral("database")))
    {
        m_database = request.value(QStringLiteral("database")).toString();
    }

    // Extract save mode for 'save' action
    if (request.contains(QStringLiteral("save_mode")))
    {
        QString sSaveMode = request.value(QStringLiteral("save_mode")).toString();
        bool bSaveModeAsInt = false; int iSaveMode = sSaveMode.toInt(&bSaveModeAsInt);
        if (bSaveModeAsInt)
        {
            m_eSaveMode = static_cast<qx::dao::save_mode::e_save_mode>(iSaveMode);
        }
        else
        {
            m_eSaveMode = ((sSaveMode == QStringLiteral("check_insert_or_update")) ? qx::dao::save_mode::e_check_insert_or_update : ((sSaveMode == QStringLiteral("insert_only")) ? qx::dao::save_mode::e_insert_only : ((sSaveMode == QStringLiteral("update_only")) ? qx::dao::save_mode::e_update_only : qx::dao::save_mode::e_none)));
        }
    }

    // Extract query
    if (request.contains(QStringLiteral("query")))
    {
        qx::cvt::from_json(request.value(QStringLiteral("query")), m_qxQuery);
    }
    else if (!m_query.isEmpty())
    {
        m_qxQuery = qx_query(m_query);
    }

    return true;
}

bool QxRestApi::QxRestApiImpl::checkRequest()
{
    bool isEntityRequired = ((m_action == QStringLiteral("get_meta_data")) || (m_action == QStringLiteral("call_entity_function")));
    isEntityRequired = (isEntityRequired || (m_action == QStringLiteral("fetch_by_id")) || (m_action == QStringLiteral("fetch_all")) || (m_action == QStringLiteral("fetch_by_query")));
    isEntityRequired = (isEntityRequired || (m_action == QStringLiteral("insert")) || (m_action == QStringLiteral("update")) || (m_action == QStringLiteral("save")));
    isEntityRequired = (isEntityRequired || (m_action == QStringLiteral("delete_by_id")) || (m_action == QStringLiteral("delete_all")) || (m_action == QStringLiteral("delete_by_query")));
    isEntityRequired = (isEntityRequired || (m_action == QStringLiteral("destroy_by_id")) || (m_action == QStringLiteral("destroy_all")) || (m_action == QStringLiteral("destroy_by_query")));
    isEntityRequired = (isEntityRequired || (m_action == QStringLiteral("exec_custom_query")) || (m_action == QStringLiteral("exist")) || (m_action == QStringLiteral("validate")) || (m_action == QStringLiteral("count")));
    if (isEntityRequired && (m_entity.isEmpty()))
    {
        buildError(9999, QStringLiteral("Parameter 'entity' is required and cannot be empty for action '%1'").arg(m_action)); return false;
    }

    bool isDataRequired = ((m_action == QStringLiteral("fetch_by_id")) || (m_action == QStringLiteral("delete_by_id")) || (m_action == QStringLiteral("destroy_by_id")));
    isDataRequired = (isDataRequired || (m_action == QStringLiteral("insert")) || (m_action == QStringLiteral("update")) || (m_action == QStringLiteral("save")));
    isDataRequired = (isDataRequired || (m_action == QStringLiteral("exist")) || (m_action == QStringLiteral("validate")));
    if (isDataRequired && (m_dataJson.isNull()))
    {
        buildError(9999, QStringLiteral("Parameter 'data' is required and cannot be empty for action '%1'").arg(m_action)); return false;
    }

    bool isQueryRequired = ((m_action == QStringLiteral("fetch_by_query")) || (m_action == QStringLiteral("delete_by_query")) || (m_action == QStringLiteral("destroy_by_query")));
    isQueryRequired = (isQueryRequired || (m_action == QStringLiteral("exec_custom_query")) || (m_action == QStringLiteral("call_custom_query")));
    if (isQueryRequired && (m_qxQuery.query().isEmpty()))
    {
        buildError(9999, QStringLiteral("Parameter 'query' is required and cannot be empty for action '%1'").arg(m_action)); return false;
    }

    bool isFunctionRequired = (m_action == QStringLiteral("call_entity_function"));
    if (isFunctionRequired && (m_function.isEmpty()))
    {
        buildError(9999, QStringLiteral("Parameter 'fct' is required and cannot be empty for action '%1'").arg(m_action)); return false;
    }

    return true;
}

bool QxRestApi::QxRestApiImpl::executeAction()
{
    m_error = QSqlError();
    m_errorJson = QJsonValue();

    try
    {
        if (m_action == QStringLiteral("count"))
        {
            m_countResult = 0; m_error = m_instance->qxCount(m_countResult, m_qxQuery, (&m_db), m_relations);
        }
        else if (m_action == QStringLiteral("fetch_by_id"))
        {
            QVariant id; m_error = m_instance->qxFetchById(id, m_columns, m_relations, (&m_db));
        }
        else if (m_action == QStringLiteral("fetch_all"))
        {
            m_error = m_instance->qxFetchAll(NULL, m_columns, m_relations, (&m_db));
        }
        else if (m_action == QStringLiteral("fetch_by_query"))
        {
            m_error = m_instance->qxFetchByQuery(m_qxQuery, NULL, m_columns, m_relations, (&m_db));
        }
        else if (m_action == QStringLiteral("insert"))
        {
            m_error = m_instance->qxInsert(m_relations, (&m_db));
        }
        else if (m_action == QStringLiteral("update"))
        {
            m_error = m_instance->qxUpdate(m_qxQuery, m_columns, m_relations, (&m_db));
        }
        else if (m_action == QStringLiteral("save"))
        {
            m_error = m_instance->qxSave(m_relations, (&m_db), m_eSaveMode);
        }
        else if (m_action == QStringLiteral("delete_by_id"))
        {
            QVariant id; m_error = m_instance->qxDeleteById(id, (&m_db));
        }
        else if (m_action == QStringLiteral("delete_all"))
        {
            m_error = m_instance->qxDeleteAll(&m_db);
        }
        else if (m_action == QStringLiteral("delete_by_query"))
        {
            m_error = m_instance->qxDeleteByQuery(m_qxQuery, (&m_db));
        }
        else if (m_action == QStringLiteral("destroy_by_id"))
        {
            QVariant id; m_error = m_instance->qxDestroyById(id, (&m_db));
        }
        else if (m_action == QStringLiteral("destroy_all"))
        {
            m_error = m_instance->qxDestroyAll(&m_db);
        }
        else if (m_action == QStringLiteral("destroy_by_query"))
        {
            m_error = m_instance->qxDestroyByQuery(m_qxQuery, (&m_db));
        }
        else if (m_action == QStringLiteral("exec_custom_query"))
        {
            m_error = m_instance->qxExecuteQuery(m_qxQuery, NULL, (&m_db));
        }
        else if (m_action == QStringLiteral("exist"))
        {
            QVariant id; m_existResult = m_instance->qxExist(id, (&m_db));
        }
        else if (m_action == QStringLiteral("validate"))
        {
            m_validateResult = m_instance->qxValidate();
        }
        else if (m_action == QStringLiteral("call_custom_query"))
        {
            m_error = qx::dao::call_query(m_qxQuery, (&m_db));
        }
        else if (m_action == QStringLiteral("call_entity_function"))
        {
            if (!callEntityFunction())
            {
                return false;
            }
        }
        else if (m_action == QStringLiteral("get_meta_data"))
        {
            getMetaData();
        }
        else if (m_action == QStringLiteral("get_databases"))
        {
            getDatabases();
        }
        else
        {
            buildError(9999, QStringLiteral("Unknown action '%1'").arg(m_action));
            return false;
        }
    }
    catch (const qx::exception& x)
    {
        buildError(9999, QStringLiteral("An exception occurred executing action '%1' : %2 - %3").arg(m_action).arg(x.getCode()).arg(x.getDescription()));
        return false;
    }
    catch (const std::exception& e)
    {
        buildError(9999, QStringLiteral("An exception occurred executing action '%1' : %2").arg(m_action).arg(QLatin1String(e.what())));
        return false;
    }
    catch (...)
    {
        buildError(9999, QStringLiteral("Unknown exception occurred executing action '%1'").arg(m_action));
        return false;
    }

    if (m_error.isValid())
    {
        buildError(m_error);
    }
    return (!m_error.isValid());
}

bool QxRestApi::QxRestApiImpl::buildResponse()
{
    if (m_action == QStringLiteral("get_meta_data"))
    {
        return m_errorJson.isNull();
    }
    if (m_action == QStringLiteral("get_databases"))
    {
        return m_errorJson.isNull();
    }

    QJsonObject response;
    if (m_action == QStringLiteral("count"))
    {
        response.insert(QStringLiteral("count"), static_cast<double>(m_countResult)); m_responseJson = response; return true;
    }
    else if (m_action == QStringLiteral("exist"))
    {
        response.insert(QStringLiteral("exist"), (m_existResult ? true : false)); m_responseJson = response; return true;
    }
    else if (m_action == QStringLiteral("validate"))
    {
        response.insert(QStringLiteral("invalid_values"), qx::cvt::to_json(m_validateResult)); m_responseJson = response; return true;
    }
    else if (m_action == QStringLiteral("call_custom_query"))
    {
        response.insert(QStringLiteral("query_output"), qx::cvt::to_json(m_qxQuery)); m_responseJson = response; return true;
    }
    else if ((m_action == QStringLiteral("delete_all")) || (m_action == QStringLiteral("delete_by_query")))
    {
        response.insert(QStringLiteral("deleted"), true); m_responseJson = response; return true;
    }
    else if ((m_action == QStringLiteral("destroy_all")) || (m_action == QStringLiteral("destroy_by_query")))
    {
        response.insert(QStringLiteral("destroyed"), true); m_responseJson = response; return true;
    }
    else if (m_action == QStringLiteral("call_entity_function"))
    {
        return true;
    }

    QString outputFormat = (m_outputFormat.isEmpty() ? QString() : QStringLiteral("filter: ") + m_outputFormat.join(QStringLiteral(" | ")));
    if (outputFormat.isEmpty() && ((m_action == QStringLiteral("insert")) || (m_action == QStringLiteral("update")) || (m_action == QStringLiteral("save"))))
    {
        outputFormat = QX_JSON_SERIALIZE_ONLY_ID;
    }
    else if (outputFormat.isEmpty() && ((m_action == QStringLiteral("delete_by_id")) || (m_action == QStringLiteral("destroy_by_id"))))
    {
        outputFormat = QX_JSON_SERIALIZE_ONLY_ID;
    }
    m_responseJson = m_instance->toJson_(outputFormat);
    return true;
}

bool QxRestApi::QxRestApiImpl::formatResponse()
{
    QJsonObject response;
    if (!m_requestId.isEmpty())
    {
        response.insert(QStringLiteral("request_id"), m_requestId);
    }
    response.insert(QStringLiteral("data"), m_responseJson);
    m_responseJson = response;
    return true;
}

qx_bool QxRestApi::QxRestApiImpl::callEntityFunction()
{
    qx::any anyResponse;
    std::vector<qx::any> anyRequest;
    anyRequest.push_back(m_dataJson);
    if (m_entity.isEmpty())
    {
        buildError(9999, QStringLiteral("Unable to call entity function : 'entity' field is empty")); return qx_bool(false);
    }
    if (m_function.isEmpty())
    {
        buildError(9999, QStringLiteral("Unable to call entity function : 'fct' field is empty")); return qx_bool(false);
    }
    if (!qx::QxClassX::getFctStatic(m_entity, m_function, true))
    {
        buildError(9999, QStringLiteral("Unable to call entity function : '%1::%2' function not found (or not registered in QxOrm context)").arg(m_entity).arg(m_function));
        return qx_bool(false);
    }
    qx_bool bInvokeFct = qx::QxClassX::invokeStatic(m_entity, m_function, anyRequest, (&anyResponse));
    if (bInvokeFct)
    {
        m_responseJson = qx::any_cast<QJsonValue>(anyResponse);
    }
    else
    {
        buildError(static_cast<int>(bInvokeFct.getCode()), bInvokeFct.getDesc());
    }
    return bInvokeFct;
}

void QxRestApi::QxRestApiImpl::getMetaData()
{
    QJsonObject response;
    QxClassX::registerAllClasses(true);
    if (m_entity == QStringLiteral("*"))
    {
        QJsonArray entities;
        QxCollection<QString, IxClass*>* pAllClasses = QxClassX::getAllClasses();
        if (!pAllClasses)
        {
            buildError(9999, QStringLiteral("Unable to access to registered classes"));
            return;
        }
        for (auto itr = pAllClasses->begin(); itr != pAllClasses->end(); ++itr)
        {
            IxClass* pClass = itr->second; if (!pClass || !pClass->implementIxPersistable())
            {
                continue;
            }
            entities.append(getMetaData(pClass));
        }
        response.insert(QStringLiteral("entities"), entities);
    }
    else
    {
        IxClass* pClass = QxClassX::getClass(m_entity);
        if (!pClass)
        {
            buildError(9999, QStringLiteral("Entity not found : ") + m_entity);
            return;
        }
        if (!pClass->implementIxPersistable())
        {
            buildError(9999, QStringLiteral("Entity doesn't implement qx::IxPersistable interface : ") + m_entity);
            return;
        }
        response.insert(QStringLiteral("entity"), getMetaData(pClass));
    }
    m_responseJson = response;
}

QJsonValue QxRestApi::QxRestApiImpl::getMetaData(IxClass* pClass)
{
    // Fill data about entity
    QJsonObject entity;
    entity.insert(QStringLiteral("key"), pClass->getKey());
    entity.insert(QStringLiteral("name"), pClass->getName());
    entity.insert(QStringLiteral("description"), pClass->getDescription());
    entity.insert(QStringLiteral("version"), static_cast<int>(pClass->getVersion()));
    entity.insert(QStringLiteral("base_entity"), (pClass->getBaseClass() ? pClass->getBaseClass()->getKey() : QString()));

    // Fill primary key information
    IxDataMember* pDataMemberId = pClass->getId();
    QJsonObject primaryKey;
    if (pDataMemberId)
    {
        primaryKey.insert(QStringLiteral("key"), pDataMemberId->getKey());
        primaryKey.insert(QStringLiteral("description"), pDataMemberId->getDescription());
        primaryKey.insert(QStringLiteral("type"), pDataMemberId->getType());
    }
    entity.insert(QStringLiteral("entity_id"), primaryKey);

    // Fill list of properties
    QJsonArray properties;
    IxDataMemberX* pDataMemberX = pClass->getDataMemberX();
    if (pDataMemberX)
    {
        for (long l = 0; l < pDataMemberX->count(); l++)
        {
            IxDataMember* p = pDataMemberX->get(l); if (!p || (p == pDataMemberId))
            {
                continue;
            }
            IxSqlRelation* pRelation = p->getSqlRelation(); if (pRelation)
            {
                continue;
            }
            QJsonObject property;
            property.insert(QStringLiteral("key"), p->getKey());
            property.insert(QStringLiteral("description"), p->getDescription());
            property.insert(QStringLiteral("type"), p->getType());
            properties.append(property);
        }
    }
    entity.insert(QStringLiteral("properties"), properties);

    // Fill list of relationships
    QJsonArray relations;
    if (pDataMemberX)
    {
        for (long l = 0; l < pDataMemberX->count(); l++)
        {
            IxDataMember* p = pDataMemberX->get(l); if (!p || (p == pDataMemberId))
            {
                continue;
            }
            IxSqlRelation* pRelation = p->getSqlRelation(); if (!pRelation)
            {
                continue;
            }
            QJsonObject relation;
            relation.insert(QStringLiteral("key"), p->getKey());
            relation.insert(QStringLiteral("description"), p->getDescription());
            relation.insert(QStringLiteral("type"), p->getType());
            relation.insert(QStringLiteral("type_relation"), pRelation->getDescription());
            relation.insert(QStringLiteral("target"), (pRelation->getClass() ? pRelation->getClass()->getKey() : QString()));
            relations.append(relation);
        }
    }
    entity.insert(QStringLiteral("relations"), relations);

    return entity;
}

void QxRestApi::QxRestApiImpl::getDatabases()
{}

#endif // _QX_NO_JSON

} // namespace qx
