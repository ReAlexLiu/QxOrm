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

#include <QxDataMember/IxDataMember.h>
#include <QxDataMember/IxDataMemberX.h>

#include <QxDao/QxSqlDatabase.h>
#include <QxDao/IxSqlRelation.h>

#include <QxSerialize/QxSerialize.h>

#include <QxConvert/QxConvert_Impl.h>

#include <QxRegister/QxRegister.h>
#include <QxRegister/IxClass.h>

#include <QxValidator/IxValidatorX.h>

#include <QxMemLeak/mem_leak.h>

#define QX_CONSTRUCT_IX_DATA_MEMBER() \
m_iPrecision(6), m_iMinLength(-1), m_iMaxLength(-1), m_bRequired(false), \
m_bReadOnly(false), m_bAutoIncrement(false), m_bNotNull(false), \
m_bIsPrimaryKey(false), m_bAccessDataPointer(false), m_bIndex(false), \
m_bUnique(false), m_pName(NULL), m_pParent(NULL)

namespace qx
{

struct IxDataMember::IxDataMemberImpl
{

    typedef QMap<int, QPair<IxSqlRelation*, int> > type_fk_part_of_pk;
    typedef std::unique_ptr<type_fk_part_of_pk> type_fk_part_of_pk_ptr;  //!< Type used by primary key to manage the case where a foreign key (relationship) is also a part of primary key (which can contain several columns)
    typedef QMap<int, QPair<IxDataMember*, int> > type_part_of_pk;
    typedef std::unique_ptr<type_part_of_pk> type_part_of_pk_ptr;        //!< Type used by relationship to manage the case where a foreign key (relationship) is also a part of primary key (which can contain several columns)

    QString     m_sKey;                 //!< Data key
    QString     m_sName;                //!< Data name <=> database record name (if empty => data key)
    QString     m_sNameParent;          //!< Data parent name <=> database table name
    QString     m_sDescription;         //!< Data description
    QString     m_sFormat;              //!< Data format ('%04d' for example)
    QString     m_sSqlType;             //!< Data sql type
    QString     m_sSqlAlias;            //!< Data sql alias
    long        m_lVersion;             //!< Data version creation
    bool        m_bSerialize;           //!< Data must be serialized
    bool        m_bDao;                 //!< Data is associated with a data source
    QVariant    m_vDefaultValue;        //!< Data default value under QVariant format
    QVariant    m_vMinValue;            //!< Data minimum value under QVariant format
    QVariant    m_vMaxValue;            //!< Data maximum value under QVariant format
    int         m_iPrecision;           //!< Data precision for numerics values (double, float, etc...)
    int         m_iMinLength;           //!< Data minimum length (-1 <=> no min length)
    int         m_iMaxLength;           //!< Data maximum length (-1 <=> no max length)
    bool        m_bRequired;            //!< Data is required or optional
    bool        m_bReadOnly;            //!< Data is read-only
    bool        m_bAutoIncrement;       //!< Data value is auto-generated (auto-increment)
    bool        m_bNotNull;             //!< Data can be null or not
    bool        m_bIsPrimaryKey;        //!< Data is a primary key
    bool        m_bAccessDataPointer;   //!< Can access to the data-member pointer
    bool        m_bIndex;               //!< Data is an index to optimize SQL queries
    bool        m_bUnique;              //!< Data is unique : 2 rows cannot have the same value for this column

    QByteArray m_byteName;              //!< Optimization to retrieve name under "const char *" format
    const char* m_pName;               //!< Optimization to retrieve name under "const char *" format
    QStringList m_lstNames;             //!< Particular case of "boost::tuple<>" data member (multi-column primary key, composite key)

    std::unique_ptr<IxSqlRelation> m_pSqlRelation;     //!< Sql relation to build/resolve sql query
    IxDataMemberX* m_pParent;                         //!< 'IxDataMemberX' parent

    type_fk_part_of_pk_ptr m_pListRelationPartOfPrimaryKey;     //!< Used by primary key to manage the case where a foreign key (relationship) is also a part of primary key (which can contain several columns)
    type_part_of_pk_ptr m_pListPartOfPrimaryKey;                //!< Used by relationship to manage the case where a foreign key (relationship) is also a part of primary key (which can contain several columns)

    IxDataMemberImpl(const QString& sKey, long lVersion, bool bSerialize, bool bDao) : m_sKey(sKey), m_lVersion(lVersion), m_bSerialize(bSerialize), m_bDao(bDao), QX_CONSTRUCT_IX_DATA_MEMBER()
    {
        qAssert(!m_sKey.isEmpty()); updateNamePtr();
    }
    ~IxDataMemberImpl()
    {
        ;
    }

    void updateNamePtr()
    {
        QString sNamePtr = (m_sName.isEmpty() ? m_sKey : m_sName);
        sNamePtr.replace(QStringLiteral("|"), QStringLiteral("-")); // valid xml tag
        m_byteName = sNamePtr.toLatin1();
        m_pName = m_byteName.constData();
        m_lstNames = (m_sName.isEmpty() ? m_sKey.split(QStringLiteral("|")) : m_sName.split(QStringLiteral("|")));
    }

};

IxDataMember::IxDataMember(const QString& sKey) : qx::QxPropertyBag(), m_pImpl(new IxDataMemberImpl(sKey, -1, true, true))
{
    ;
}

IxDataMember::IxDataMember(const QString& sKey, long lVersion, bool bSerialize, bool bDao) : qx::QxPropertyBag(), m_pImpl(new IxDataMemberImpl(sKey, lVersion, bSerialize, bDao))
{
    ;
}

IxDataMember::~IxDataMember()
{
    ;
}

#ifdef _QX_ENABLE_BOOST_SERIALIZATION
template<class Archive>
void IxDataMember::serialize(Archive& ar, const unsigned int version)
{
    Q_UNUSED(version);
    ar& boost::serialization::make_nvp(QStringLiteral("key"), m_pImpl->m_sKey);
    ar& boost::serialization::make_nvp("name", m_pImpl->m_sName);
    ar& boost::serialization::make_nvp("nameParent", m_pImpl->m_sNameParent);
    ar& boost::serialization::make_nvp(QStringLiteral("desc"), m_pImpl->m_sDescription);
    ar& boost::serialization::make_nvp("format", m_pImpl->m_sFormat);
    ar& boost::serialization::make_nvp("sqlType", m_pImpl->m_sSqlType);
    ar& boost::serialization::make_nvp("sqlAlias", m_pImpl->m_sSqlAlias);
    ar& boost::serialization::make_nvp("version", m_pImpl->m_lVersion);
    ar& boost::serialization::make_nvp("serialize", m_pImpl->m_bSerialize);
    ar& boost::serialization::make_nvp("dao", m_pImpl->m_bDao);
    ar& boost::serialization::make_nvp("defaultValue", m_pImpl->m_vDefaultValue);
    ar& boost::serialization::make_nvp("minValue", m_pImpl->m_vMinValue);
    ar& boost::serialization::make_nvp("maxValue", m_pImpl->m_vMaxValue);
    ar& boost::serialization::make_nvp("precision", m_pImpl->m_iPrecision);
    ar& boost::serialization::make_nvp("minLength", m_pImpl->m_iMinLength);
    ar& boost::serialization::make_nvp("maxLength", m_pImpl->m_iMaxLength);
    ar& boost::serialization::make_nvp("required", m_pImpl->m_bRequired);
    ar& boost::serialization::make_nvp("readOnly", m_pImpl->m_bReadOnly);
    ar& boost::serialization::make_nvp("autoIncrement", m_pImpl->m_bAutoIncrement);
    ar& boost::serialization::make_nvp("notNull", m_pImpl->m_bNotNull);
    ar& boost::serialization::make_nvp("isPrimaryKey", m_pImpl->m_bIsPrimaryKey);
    ar& boost::serialization::make_nvp("isIndex", m_pImpl->m_bIndex);
    ar& boost::serialization::make_nvp("isUnique", m_pImpl->m_bUnique);
}
#endif // _QX_ENABLE_BOOST_SERIALIZATION

QString IxDataMember::getKey() const
{
    return m_pImpl->m_sKey;
}

QString IxDataMember::getName() const
{
    return (m_pImpl->m_sName.isEmpty() ? m_pImpl->m_sKey : m_pImpl->m_sName);
}

int IxDataMember::getNameCount() const
{
    return m_pImpl->m_lstNames.count();
}

QString IxDataMember::getNameParent() const
{
    return m_pImpl->m_sNameParent;
}

const char* IxDataMember::getNamePtr() const
{
    return m_pImpl->m_pName;
}

QString IxDataMember::getDescription() const
{
    return m_pImpl->m_sDescription;
}

QString IxDataMember::getFormat() const
{
    return m_pImpl->m_sFormat;
}

long IxDataMember::getVersion() const
{
    return m_pImpl->m_lVersion;
}

bool IxDataMember::getSerialize() const
{
    return m_pImpl->m_bSerialize;
}

bool IxDataMember::getDao() const
{
    return m_pImpl->m_bDao;
}

QVariant IxDataMember::getDefaultValue() const
{
    return m_pImpl->m_vDefaultValue;
}

QVariant IxDataMember::getMinValue() const
{
    return m_pImpl->m_vMinValue;
}

QVariant IxDataMember::getMaxValue() const
{
    return m_pImpl->m_vMaxValue;
}

int IxDataMember::getPrecision() const
{
    return m_pImpl->m_iPrecision;
}

int IxDataMember::getMinLength() const
{
    return m_pImpl->m_iMinLength;
}

int IxDataMember::getMaxLength() const
{
    return m_pImpl->m_iMaxLength;
}

bool IxDataMember::getRequired() const
{
    return m_pImpl->m_bRequired;
}

bool IxDataMember::getReadOnly() const
{
    return m_pImpl->m_bReadOnly;
}

bool IxDataMember::getAutoIncrement() const
{
    return m_pImpl->m_bAutoIncrement;
}

bool IxDataMember::getNotNull() const
{
    return m_pImpl->m_bNotNull;
}

bool IxDataMember::getIsPrimaryKey() const
{
    return m_pImpl->m_bIsPrimaryKey;
}

bool IxDataMember::getIsIndex() const
{
    return m_pImpl->m_bIndex;
}

bool IxDataMember::getIsUnique() const
{
    return m_pImpl->m_bUnique;
}

IxDataMemberX* IxDataMember::getParent() const
{
    return m_pImpl->m_pParent;
}

IxSqlRelation* IxDataMember::getSqlRelation() const
{
    return m_pImpl->m_pSqlRelation.get();
}

bool IxDataMember::hasSqlRelation() const
{
    return (m_pImpl->m_pSqlRelation.get() != NULL);
}

bool IxDataMember::getAccessDataPointer() const
{
    return m_pImpl->m_bAccessDataPointer;
}

QString IxDataMember::getType() const
{
    return QStringLiteral("");
}

QString IxDataMember::getTypeParent() const
{
    IxClass* pClass = (m_pImpl->m_pParent ? m_pImpl->m_pParent->getClass() : NULL); return (pClass ? pClass->getKey() : QString());
}

void IxDataMember::setName(const QString& sName)
{
    m_pImpl->m_sName = sName; m_pImpl->updateNamePtr();
}

void IxDataMember::setNameParent(const QString& sName)
{
    m_pImpl->m_sNameParent = sName;
}

void IxDataMember::setDescription(const QString& sDesc)
{
    m_pImpl->m_sDescription = sDesc;
}

void IxDataMember::setFormat(const QString& sFormat)
{
    m_pImpl->m_sFormat = sFormat;
}

void IxDataMember::setSqlType(const QString& sSqlType)
{
    m_pImpl->m_sSqlType = sSqlType;
}

void IxDataMember::setSqlAlias(const QString& sSqlAlias)
{
    m_pImpl->m_sSqlAlias = sSqlAlias;
}

void IxDataMember::setVersion(long lVersion)
{
    m_pImpl->m_lVersion = lVersion;
}

void IxDataMember::setSerialize(bool bSerialize)
{
    m_pImpl->m_bSerialize = bSerialize;
}

void IxDataMember::setDao(bool bDao)
{
    m_pImpl->m_bDao = bDao;
}

void IxDataMember::setDefaultValue(const QVariant& var)
{
    m_pImpl->m_vDefaultValue = var;
}

void IxDataMember::setPrecision(int iPrecision)
{
    m_pImpl->m_iPrecision = iPrecision;
}

void IxDataMember::setRequired(bool bRequired)
{
    m_pImpl->m_bRequired = bRequired;
}

void IxDataMember::setReadOnly(bool bReadOnly)
{
    m_pImpl->m_bReadOnly = bReadOnly;
}

void IxDataMember::setAutoIncrement(bool bAutoIncrement)
{
    m_pImpl->m_bAutoIncrement = bAutoIncrement;
}

void IxDataMember::setIsPrimaryKey(bool bIsPrimaryKey)
{
    m_pImpl->m_bIsPrimaryKey = bIsPrimaryKey;
}

void IxDataMember::setIsIndex(bool bIsIndex)
{
    m_pImpl->m_bIndex = bIsIndex;
}

void IxDataMember::setIsUnique(bool bIsUnique)
{
    m_pImpl->m_bUnique = bIsUnique;
}

void IxDataMember::setParent(IxDataMemberX* pParent)
{
    m_pImpl->m_pParent = pParent;
}

void IxDataMember::setSqlRelation(IxSqlRelation* pSqlRelation)
{
    m_pImpl->m_pSqlRelation.reset(pSqlRelation);
}

void IxDataMember::setAccessDataPointer(bool b)
{
    m_pImpl->m_bAccessDataPointer = b;
}

QVariant IxDataMember::toVariant(const void* pOwner, int iIndexName /* = -1 */, qx::cvt::context::ctx_type ctx /* = qx::cvt::context::e_no_context */) const
{
    return this->toVariant(pOwner, m_pImpl->m_sFormat, iIndexName, ctx);
}

qx_bool IxDataMember::fromVariant(void* pOwner, const QVariant& v, int iIndexName /* = -1 */, qx::cvt::context::ctx_type ctx /* = qx::cvt::context::e_no_context */)
{
    return this->fromVariant(pOwner, v, m_pImpl->m_sFormat, iIndexName, ctx);
}

#ifndef _QX_NO_JSON

QJsonValue IxDataMember::toJson(const void* pOwner) const
{
    return this->toJson(pOwner, m_pImpl->m_sFormat);
}

qx_bool IxDataMember::fromJson(void* pOwner, const QJsonValue& j)
{
    return this->fromJson(pOwner, j, m_pImpl->m_sFormat);
}

#endif // _QX_NO_JSON

void IxDataMember::setMinValue(long lMinValue, const QString& sMessage /* = QString() */)
{
    m_pImpl->m_vMinValue = (qlonglong)lMinValue;
    IxClass* pClass = (m_pImpl->m_pParent ? m_pImpl->m_pParent->getClass() : NULL); if (!pClass)
    {
        return;
    }
    IxValidatorX* pAllValidator = pClass->getAllValidator(); if (!pAllValidator)
    {
        return;
    }
    pAllValidator->add_MinValue(m_pImpl->m_sKey, lMinValue, sMessage);
}

void IxDataMember::setMinValue(double dMinValue, const QString& sMessage /* = QString() */)
{
    m_pImpl->m_vMinValue = dMinValue;
    IxClass* pClass = (m_pImpl->m_pParent ? m_pImpl->m_pParent->getClass() : NULL); if (!pClass)
    {
        return;
    }
    IxValidatorX* pAllValidator = pClass->getAllValidator(); if (!pAllValidator)
    {
        return;
    }
    pAllValidator->add_MinDecimal(m_pImpl->m_sKey, dMinValue, sMessage);
}

void IxDataMember::setMaxValue(long lMaxValue, const QString& sMessage /* = QString() */)
{
    m_pImpl->m_vMaxValue = (qlonglong)lMaxValue;
    IxClass* pClass = (m_pImpl->m_pParent ? m_pImpl->m_pParent->getClass() : NULL); if (!pClass)
    {
        return;
    }
    IxValidatorX* pAllValidator = pClass->getAllValidator(); if (!pAllValidator)
    {
        return;
    }
    pAllValidator->add_MaxValue(m_pImpl->m_sKey, lMaxValue, sMessage);
}

void IxDataMember::setMaxValue(double dMaxValue, const QString& sMessage /* = QString() */)
{
    m_pImpl->m_vMaxValue = dMaxValue;
    IxClass* pClass = (m_pImpl->m_pParent ? m_pImpl->m_pParent->getClass() : NULL); if (!pClass)
    {
        return;
    }
    IxValidatorX* pAllValidator = pClass->getAllValidator(); if (!pAllValidator)
    {
        return;
    }
    pAllValidator->add_MaxDecimal(m_pImpl->m_sKey, dMaxValue, sMessage);
}

void IxDataMember::setMinLength(int iMinLength, const QString& sMessage /* = QString() */)
{
    m_pImpl->m_iMinLength = iMinLength;
    IxClass* pClass = (m_pImpl->m_pParent ? m_pImpl->m_pParent->getClass() : NULL); if (!pClass)
    {
        return;
    }
    IxValidatorX* pAllValidator = pClass->getAllValidator(); if (!pAllValidator)
    {
        return;
    }
    pAllValidator->add_MinLength(m_pImpl->m_sKey, (long)m_pImpl->m_iMinLength, sMessage);
}

void IxDataMember::setMaxLength(int iMaxLength, const QString& sMessage /* = QString() */)
{
    m_pImpl->m_iMaxLength = iMaxLength;
    IxClass* pClass = (m_pImpl->m_pParent ? m_pImpl->m_pParent->getClass() : NULL); if (!pClass)
    {
        return;
    }
    IxValidatorX* pAllValidator = pClass->getAllValidator(); if (!pAllValidator)
    {
        return;
    }
    pAllValidator->add_MaxLength(m_pImpl->m_sKey, (long)m_pImpl->m_iMaxLength, sMessage);
}

void IxDataMember::setNotNull(bool bNotNull, const QString& sMessage /* = QString() */)
{
    m_pImpl->m_bNotNull = bNotNull;
    IxClass* pClass = (m_pImpl->m_pParent ? m_pImpl->m_pParent->getClass() : NULL); if (!pClass)
    {
        return;
    }
    IxValidatorX* pAllValidator = pClass->getAllValidator(); if (!pAllValidator)
    {
        return;
    }
    if (m_pImpl->m_bNotNull)
    {
        pAllValidator->add_NotNull(m_pImpl->m_sKey, sMessage);
    }
}

bool IxDataMember::isThereRelationPartOfPrimaryKey(int iIndexNamePK, IxSqlRelation*& pRelation, int& iIndexNameFK) const
{
    pRelation = NULL; iIndexNameFK = -1;
    if (!m_pImpl->m_pListRelationPartOfPrimaryKey)
    {
        return false;
    }
    if (!m_pImpl->m_pListRelationPartOfPrimaryKey->contains(iIndexNamePK))
    {
        return false;
    }
    QPair<IxSqlRelation*, int> pair = m_pImpl->m_pListRelationPartOfPrimaryKey->value(iIndexNamePK);
    pRelation = pair.first; iIndexNameFK = pair.second;
    return true;
}

bool IxDataMember::isPartOfPrimaryKey(int iIndexNameFK, IxDataMember*& pPrimaryKey, int& iIndexNamePK) const
{
    pPrimaryKey = NULL; iIndexNamePK = -1;
    if (!m_pImpl->m_pListPartOfPrimaryKey)
    {
        return false;
    }
    if (!m_pImpl->m_pListPartOfPrimaryKey->contains(iIndexNameFK))
    {
        return false;
    }
    QPair<IxDataMember*, int> pair = m_pImpl->m_pListPartOfPrimaryKey->value(iIndexNameFK);
    pPrimaryKey = pair.first; iIndexNamePK = pair.second;
    return true;
}

void IxDataMember::setRelationPartOfPrimaryKey(int iIndexNamePK, IxSqlRelation* pRelation, int iIndexNameFK)
{
    if (!m_pImpl->m_pListRelationPartOfPrimaryKey)
    {
        m_pImpl->m_pListRelationPartOfPrimaryKey.reset(new IxDataMemberImpl::type_fk_part_of_pk());
    }
    if (m_pImpl->m_pListRelationPartOfPrimaryKey->contains(iIndexNamePK))
    {
        return;
    }
    QPair<IxSqlRelation*, int> pair;
    pair.first = pRelation;
    pair.second = iIndexNameFK;
    m_pImpl->m_pListRelationPartOfPrimaryKey->insert(iIndexNamePK, pair);
}

void IxDataMember::setPartOfPrimaryKey(int iIndexNameFK, IxDataMember* pPrimaryKey, int iIndexNamePK)
{
    if (!m_pImpl->m_pListPartOfPrimaryKey)
    {
        m_pImpl->m_pListPartOfPrimaryKey.reset(new IxDataMemberImpl::type_part_of_pk());
    }
    if (m_pImpl->m_pListPartOfPrimaryKey->contains(iIndexNameFK))
    {
        return;
    }
    QPair<IxDataMember*, int> pair;
    pair.first = pPrimaryKey;
    pair.second = iIndexNamePK;
    m_pImpl->m_pListPartOfPrimaryKey->insert(iIndexNameFK, pair);
}

QString IxDataMember::getName(int iIndex, const QString& sOtherName /* = QString() */) const
{
    if (!sOtherName.isEmpty())
    {
        QStringList lstOtherNames = sOtherName.split(QStringLiteral("|"));
        qAssert(m_pImpl->m_lstNames.count() == lstOtherNames.count());
        return (((iIndex >= 0) && (iIndex < lstOtherNames.count())) ? lstOtherNames.at(iIndex) : QString());
    }

    return (((iIndex >= 0) && (iIndex < m_pImpl->m_lstNames.count())) ? m_pImpl->m_lstNames.at(iIndex) : QString());
}

QString IxDataMember::getSqlAlias(const QString& sTable /* = QString() */, bool bClauseWhere /* = false */, int iIndexName /* = 0 */) const
{
    QString sTableAlias = sTable;
    sTableAlias.replace(QStringLiteral("."), QStringLiteral("_"));

    // Standard SQL disallows references to column aliases in a WHERE clause
    // cf. <http://dev.mysql.com/doc/refman/5.0/en/problems-with-alias.html>
    if (bClauseWhere && !sTableAlias.isEmpty())
    {
        return QStringLiteral("%1.%2").arg(getSqlTableName(sTableAlias)).arg(getSqlColumnName(getName(iIndexName)));
    }

    QString sSqlAlias = m_pImpl->m_sSqlAlias;
    if (!sSqlAlias.isEmpty())
    {
        return sSqlAlias;
    }
    if (!sTableAlias.isEmpty())
    {
        sSqlAlias = QStringLiteral("%1_%2_0").arg(sTableAlias).arg(getName(iIndexName));
    }
    else
    {
        sSqlAlias = QStringLiteral("%1_%2_0").arg(m_pImpl->m_sNameParent).arg(getName(iIndexName));
    }

    // Special database keywords using '[', ']' or '"' characters
    sSqlAlias.replace(QStringLiteral("["), QStringLiteral(""));
    sSqlAlias.replace(QStringLiteral("]"), QStringLiteral(""));
    sSqlAlias.replace(QStringLiteral("\""), QStringLiteral(""));
    sSqlAlias.replace(QStringLiteral("."), QStringLiteral("_"));

    return sSqlAlias;
}

QString IxDataMember::getSqlType(int iIndexName /* = -1 */) const
{
    if ((iIndexName == -1) || (getNameCount() <= 1))
    {
        return m_pImpl->m_sSqlType;
    }
    QStringList lst = m_pImpl->m_sSqlType.split(QStringLiteral("|"));
    return (((iIndexName >= 0) && (iIndexName < lst.count())) ? lst.at(iIndexName) : QString());
}

QString IxDataMember::getSqlTypeAndParams(int iIndexName /* = -1 */) const
{
    QString sResult = getSqlType(iIndexName);
    sResult += (m_pImpl->m_bNotNull ? QStringLiteral(" NOT NULL") : QStringLiteral(""));
    sResult += ((m_pImpl->m_bIsPrimaryKey && (getNameCount() <= 1)) ? QStringLiteral(" PRIMARY KEY") : QStringLiteral(""));

    if (m_pImpl->m_bAutoIncrement)
    {
        qx::dao::detail::IxSqlGenerator* pSqlGenerator = QxSqlDatabase::getSingleton()->getSqlGenerator();
        if (!pSqlGenerator)
        {
            qAssert(false); sResult += QStringLiteral(" AUTOINCREMENT");
        }
        else
        {
            sResult += QStringLiteral(" ") + pSqlGenerator->getAutoIncrement();
        }
    }

    return sResult;
}

QString IxDataMember::getSqlPlaceHolder(const QString& sAppend /* = QString() */, int iIndexName /* = 0 */, const QString& sSep /* = QStringLiteral(", ") */, const QString& sOtherName /* = QString() */, bool bCheckFKPartOfPK /* = false */) const
{
    QString sResult;
    int iIndexNameFK = 0;
    IxSqlRelation* pRelation = NULL;

    if (iIndexName == -1)
    {
        for (int i = 0; i < m_pImpl->m_lstNames.count(); i++)
        {
            if (bCheckFKPartOfPK && m_pImpl->m_bIsPrimaryKey && isThereRelationPartOfPrimaryKey(i, pRelation, iIndexNameFK))
            {
                continue;
            }
            sResult += getSqlPlaceHolder(sAppend, i, sSep, sOtherName) + sSep;
        }

        if (!sResult.isEmpty())
        {
            sResult = sResult.left(sResult.count() - sSep.count());
        } // Remove last separator
        return sResult;
    }

    // Special database keywords using '[', ']' or '"' characters
    if (bCheckFKPartOfPK && m_pImpl->m_bIsPrimaryKey && isThereRelationPartOfPrimaryKey(iIndexName, pRelation, iIndexNameFK))
    {
        return QStringLiteral("");
    }
    QString sSqlPlaceHolder = getName(iIndexName, sOtherName) + sAppend;
    sSqlPlaceHolder.replace(QStringLiteral("["), QStringLiteral(""));
    sSqlPlaceHolder.replace(QStringLiteral("]"), QStringLiteral(""));
    sSqlPlaceHolder.replace(QStringLiteral("\""), QStringLiteral(""));

    switch (QxSqlDatabase::getSingleton()->getSqlPlaceHolderStyle())
    {
    case QxSqlDatabase::ph_style_question_mark:  sResult = QStringLiteral("?");                      break;
    case QxSqlDatabase::ph_style_2_point_name:   sResult = QStringLiteral(":") + sSqlPlaceHolder;    break;
    case QxSqlDatabase::ph_style_at_name:        sResult = QStringLiteral("@") + sSqlPlaceHolder;    break;
    default:                                     sResult = QStringLiteral(":") + sSqlPlaceHolder;    break;
    }

    return sResult;
}

void IxDataMember::setSqlPlaceHolder(QSqlQuery& query, void* pOwner, const QString& sAppend /* = QString() */, const QString& sOtherName /* = QString() */, bool bCheckFKPartOfPK /* = false */) const
{
    int iIndexNameFK = 0;
    IxSqlRelation* pRelation = NULL;

    for (int i = 0; i < m_pImpl->m_lstNames.count(); i++)
    {
        if (bCheckFKPartOfPK && m_pImpl->m_bIsPrimaryKey && isThereRelationPartOfPrimaryKey(i, pRelation, iIndexNameFK))
        {
            continue;
        }
        switch (QxSqlDatabase::getSingleton()->getSqlPlaceHolderStyle())
        {
        case QxSqlDatabase::ph_style_question_mark:  query.addBindValue(toVariant(pOwner, i, qx::cvt::context::e_database));                                                 break;
        case QxSqlDatabase::ph_style_2_point_name:   query.bindValue(getSqlPlaceHolder(sAppend, i, QStringLiteral(""), sOtherName), toVariant(pOwner, i, qx::cvt::context::e_database));     break;
        case QxSqlDatabase::ph_style_at_name:        query.bindValue(getSqlPlaceHolder(sAppend, i, QStringLiteral(""), sOtherName), toVariant(pOwner, i, qx::cvt::context::e_database));     break;
        default:                                     query.bindValue(getSqlPlaceHolder(sAppend, i, QStringLiteral(""), sOtherName), toVariant(pOwner, i, qx::cvt::context::e_database));     break;
        }
    }
}

QString IxDataMember::getSqlAliasEqualToPlaceHolder(const QString& sTable /* = QString() */, bool bClauseWhere /* = false */, const QString& sAppend /* = QString() */, const QString& sSep /* = QStringLiteral(" AND ") */, bool bCheckFKPartOfPK /* = false */) const
{
    QString sResult;
    int iIndexNameFK = 0;
    IxSqlRelation* pRelation = NULL;

    for (int i = 0; i < m_pImpl->m_lstNames.count(); i++)
    {
        if (bCheckFKPartOfPK && m_pImpl->m_bIsPrimaryKey && isThereRelationPartOfPrimaryKey(i, pRelation, iIndexNameFK))
        {
            continue;
        }
        sResult += QStringLiteral("%1 = %2%3").arg(getSqlAlias(sTable, bClauseWhere, i)).arg(getSqlPlaceHolder(sAppend, i)).arg(sSep);
    }

    if (!sResult.isEmpty())
    {
        sResult = sResult.left(sResult.count() - sSep.count());
    } // Remove last separator
    return sResult;
}

QString IxDataMember::getSqlNameEqualToPlaceHolder(const QString& sAppend /* = QString() */, const QString& sSep /* = QStringLiteral(" AND ") */, bool bCheckFKPartOfPK /* = false */) const
{
    QString sResult;
    int iIndexNameFK = 0;
    IxSqlRelation* pRelation = NULL;

    for (int i = 0; i < m_pImpl->m_lstNames.count(); i++)
    {
        if (bCheckFKPartOfPK && m_pImpl->m_bIsPrimaryKey && isThereRelationPartOfPrimaryKey(i, pRelation, iIndexNameFK))
        {
            continue;
        }
        sResult += QStringLiteral("%1 = %2%3").arg(getSqlColumnName(getName(i))).arg(getSqlPlaceHolder(sAppend, i)).arg(sSep);
    }

    if (!sResult.isEmpty())
    {
        sResult = sResult.left(sResult.count() - sSep.count());
    } // Remove last separator
    return sResult;
}

QString IxDataMember::getSqlTablePointNameAsAlias(const QString& sTable, const QString& sSep /* = QStringLiteral(", ") */, const QString& sSuffixAlias /* = QString() */, bool bCheckFKPartOfPK /* = false */, const QString& sCustomAlias /* = QString() */) const
{
    QString sResult;
    int iIndexNameFK = 0;
    IxSqlRelation* pRelation = NULL;
    QString sTableAlias = (sCustomAlias.isEmpty() ? sTable : sCustomAlias);
    sTableAlias.replace(QStringLiteral("."), QStringLiteral("_"));
    sTableAlias = getSqlTableName(sTableAlias);

    QString sTableAliasSep = QStringLiteral(" AS ");
    qx::dao::detail::IxSqlGenerator* pSqlGenerator = QxSqlDatabase::getSingleton()->getSqlGenerator();
    if (pSqlGenerator)
    {
        sTableAliasSep = pSqlGenerator->getTableAliasSep();
    }

    for (int i = 0; i < m_pImpl->m_lstNames.count(); i++)
    {
        if (bCheckFKPartOfPK && m_pImpl->m_bIsPrimaryKey && isThereRelationPartOfPrimaryKey(i, pRelation, iIndexNameFK))
        {
            continue;
        }
        sResult += QStringLiteral("%1.%2%3%4%5%6")
            .arg(sTableAlias)
            .arg(getSqlColumnName(getName(i)))
            .arg(sTableAliasSep)
            .arg(getSqlAlias((sCustomAlias.isEmpty() ? sTable : sCustomAlias), false, i))
            .arg(sSuffixAlias)
            .arg(sSep);
    }

    if (!sResult.isEmpty())
    {
        sResult = sResult.left(sResult.count() - sSep.count());
    } // Remove last separator
    return sResult;
}

QString IxDataMember::getSqlName(const QString& sSep /* = QStringLiteral(", ") */, const QString& sOtherName /* = QString() */, bool bCheckFKPartOfPK /* = false */) const
{
    QString sResult;
    int iIndexNameFK = 0;
    IxSqlRelation* pRelation = NULL;

    for (int i = 0; i < m_pImpl->m_lstNames.count(); i++)
    {
        if (bCheckFKPartOfPK && m_pImpl->m_bIsPrimaryKey && isThereRelationPartOfPrimaryKey(i, pRelation, iIndexNameFK))
        {
            continue;
        }
        sResult += getSqlColumnName(getName(i, sOtherName)) + sSep;
    }

    if (!sResult.isEmpty())
    {
        sResult = sResult.left(sResult.count() - sSep.count());
    } // Remove last separator
    return sResult;
}

QString IxDataMember::getSqlNameAndTypeAndParams(const QString& sSep /* = QStringLiteral(", ") */, const QString& sOtherName /* = QString() */, bool bCheckFKPartOfPK /* = false */) const
{
    QString sResult;
    int iIndexNameFK = 0;
    IxSqlRelation* pRelation = NULL;

    for (int i = 0; i < m_pImpl->m_lstNames.count(); i++)
    {
        if (bCheckFKPartOfPK && m_pImpl->m_bIsPrimaryKey && isThereRelationPartOfPrimaryKey(i, pRelation, iIndexNameFK))
        {
            continue;
        }
        sResult += QStringLiteral("%1 %2%3").arg(getSqlColumnName(getName(i, sOtherName))).arg(getSqlTypeAndParams(i)).arg(sSep);
    }

    if (!sResult.isEmpty())
    {
        sResult = sResult.left(sResult.count() - sSep.count());
    } // Remove last separator
    return sResult;
}

QString IxDataMember::getSqlFromTable(const QString& sTable, const QString& sCustomAlias /* = QString() */)
{
    if (sCustomAlias.isEmpty() && !sTable.contains(QStringLiteral(".")))
    {
        return getSqlTableName(sTable);
    }
    QString sTableAlias = (sCustomAlias.isEmpty() ? sTable : sCustomAlias);
    sTableAlias.replace(QStringLiteral("."), QStringLiteral("_"));

    QString sTableAliasSep = QStringLiteral(" AS ");
    qx::dao::detail::IxSqlGenerator* pSqlGenerator = QxSqlDatabase::getSingleton()->getSqlGenerator();
    if (pSqlGenerator)
    {
        sTableAliasSep = pSqlGenerator->getTableAliasSep();
    }

    return (getSqlTableName(sTable) + sTableAliasSep + sTableAlias);
}

QString IxDataMember::getSqlTableName(const QString& sTable)
{
    QStringList lstDelimiter = QxSqlDatabase::getSingleton()->getSqlDelimiterForTableName();
    bool bAddSquareBracketsForTableName = QxSqlDatabase::getSingleton()->getAddSqlSquareBracketsForTableName();
    QString sStart = (bAddSquareBracketsForTableName ? QStringLiteral("[") : QString());
    QString sEnd = (bAddSquareBracketsForTableName ? QStringLiteral("]") : QString());
    if (lstDelimiter.count() > 0)
    {
        sStart = lstDelimiter.at(0); sEnd = lstDelimiter.at(0);
    }
    if (lstDelimiter.count() > 1)
    {
        sEnd = lstDelimiter.at(1);
    }
    if (sStart.isEmpty() || sEnd.isEmpty() || sTable.contains(sStart) || sTable.contains(sEnd))
    {
        return sTable;
    }
    QString sResult = (sStart + sTable + sEnd); sResult.replace(QStringLiteral("."), (QStringLiteral("%1.%2").arg(sEnd).arg(sStart)));
    return sResult;
}

QString IxDataMember::getSqlColumnName(const QString& sColumn)
{
    QStringList lstDelimiter = QxSqlDatabase::getSingleton()->getSqlDelimiterForColumnName();
    bool bAddSquareBracketsForColumnName = QxSqlDatabase::getSingleton()->getAddSqlSquareBracketsForColumnName();
    QString sStart = (bAddSquareBracketsForColumnName ? QStringLiteral("[") : QString());
    QString sEnd = (bAddSquareBracketsForColumnName ? QStringLiteral("]") : QString());
    if (lstDelimiter.count() > 0)
    {
        sStart = lstDelimiter.at(0); sEnd = lstDelimiter.at(0);
    }
    if (lstDelimiter.count() > 1)
    {
        sEnd = lstDelimiter.at(1);
    }
    if (sStart.isEmpty() || sEnd.isEmpty() || sColumn.contains(sStart) || sColumn.contains(sEnd))
    {
        return sColumn;
    }
    QString sResult = (sStart + sColumn + sEnd); sResult.replace(QStringLiteral("."), (QStringLiteral("%1.%2").arg(sEnd).arg(sStart)));
    return sResult;
}

} // namespace qx

inline bool operator< (const qx::IxDataMember& i1, const qx::IxDataMember& i2)
{
    return (i1.getKey() < i2.getKey());
}
inline bool operator> (const qx::IxDataMember& i1, const qx::IxDataMember& i2)
{
    return (i1.getKey() > i2.getKey());
}
