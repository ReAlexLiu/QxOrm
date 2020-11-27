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

#ifdef _QX_ENABLE_BOOST_SERIALIZATION

namespace qx
{

template <class T, class ArchiveInput /* = QX_DEFAULT_ARCHIVE_INPUT */, class ArchiveOutput /* = QX_DEFAULT_ARCHIVE_OUTPUT */>
qx_bool QxArchive_ToFile<T, ArchiveInput, ArchiveOutput>::toFile(const T& obj, const QString& sFileName, unsigned int flags /* = boost::archive::no_header */)
{
    typedef typename qx::trait::archive_wide_traits<ArchiveOutput>::type_ofstream qx_type_ofstream;
    qx_type_ofstream ofile(qPrintable(sFileName), (std::ios_base::binary | std::ios_base::out | std::ios_base::trunc));
    if (!ofile.is_open())
    {
        return qx_bool(false, 0, QStringLiteral("Cannot create or open file '%1'").arg(sFileName));
    }
    ArchiveOutput oar(ofile, flags);
    qx_bool bSerializeOk = false;
    const char* sTag = QxClassName<T>::get_xml_tag();
    QxBoostSerializeRegisterHelperX::helper(oar);

    try
    {
        oar << boost::serialization::make_nvp(sTag, obj); 
        bSerializeOk = ofile.good();
    }
    catch (const boost::archive::archive_exception& e)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (const std::exception& e)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (...)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QStringLiteral("unknown error")));
    }

    ofile.close();
    if (!bSerializeOk.getDesc().isEmpty())
    {
        QString sMsg = QStringLiteral("qx::QxArchive_ToFile<T>::toFile() -> ") + bSerializeOk.getDesc();
        xwarn(qPrintable(sMsg));
    }
    qAssert(bSerializeOk);

    return bSerializeOk;
}

template <class T, class ArchiveInput /* = QX_DEFAULT_ARCHIVE_INPUT */, class ArchiveOutput /* = QX_DEFAULT_ARCHIVE_OUTPUT */>
qx_bool QxArchive_FromFile<T, ArchiveInput, ArchiveOutput>::fromFile(T& obj, const QString& sFileName, unsigned int flags /* = boost::archive::no_header */)
{
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_ifstream qx_type_ifstream;
    qx_type_ifstream ifile(qPrintable(sFileName), (std::ios_base::binary | std::ios_base::in));
    if (!ifile.is_open())
    {
        return qx_bool(false, 0, QStringLiteral("Cannot open file '%1'").arg(sFileName));
    }
    ArchiveInput iar(ifile, flags);
    qx_bool bDeserializeOk = false;
    const char* sTag = QxClassName<T>::get_xml_tag();
    QxBoostSerializeRegisterHelperX::helper(iar);

    try
    {
        iar >> boost::serialization::make_nvp(sTag, obj); bDeserializeOk = true;
    }
    catch (const boost::archive::archive_exception& e)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (const std::exception& e)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (...)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QStringLiteral("unknown error")));
    }

    ifile.close();
    if (!bDeserializeOk.getDesc().isEmpty())
    {
        QString sMsg = QStringLiteral("qx::QxArchive_FromFile<T>::fromFile() -> ") + bDeserializeOk.getDesc();
        xwarn(qPrintable(sMsg));
    }
    qAssert(bDeserializeOk);

    return bDeserializeOk;
}

template <class T, class ArchiveInput /* = QX_DEFAULT_ARCHIVE_INPUT */, class ArchiveOutput /* = QX_DEFAULT_ARCHIVE_OUTPUT */>
qx_bool QxArchive_ToFileCompressed<T, ArchiveInput, ArchiveOutput>::toFileCompressed(const T& obj, const QString& sFileName, unsigned int flags /* = boost::archive::no_header */, int iCompressionLevel /* = -1 */)
{
    typedef typename qx::trait::archive_wide_traits<ArchiveOutput>::type_ostringstream qx_type_ostringstream;
    qx_type_ostringstream oss(std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    ArchiveOutput oar(oss, flags);
    qx_bool bSerializeOk = false;
    const char* sTag = QxClassName<T>::get_xml_tag();
    QxBoostSerializeRegisterHelperX::helper(oar);

    try
    {
        oar << boost::serialization::make_nvp(sTag, obj); bSerializeOk = oss.good();
    }
    catch (const boost::archive::archive_exception& e)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (const std::exception& e)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (...)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QStringLiteral("unknown error")));
    }

    qAssert(bSerializeOk);
    if (!bSerializeOk.getDesc().isEmpty())
    {
        QString sMsg = QStringLiteral("qx::QxArchive_ToFileCompressed<T>::toFileCompressed() -> ") + bSerializeOk.getDesc();
        xwarn(qPrintable(sMsg));
    }
    if (!bSerializeOk)
    {
        return false;
    }

    oss.seekp(0, std::ios::end);
    QByteArray compressed = qCompress((unsigned char*)(oss.str().c_str()), static_cast<int>(oss.tellp()), iCompressionLevel);
    if (compressed.isEmpty())
    {
        qAssert(false); return qx_bool(false, 0, QStringLiteral("Cannot compress archive"));
    }

    typedef typename qx::trait::archive_wide_traits<ArchiveOutput>::type_ofstream qx_type_ofstream;
    qx_type_ofstream ofile(qPrintable(sFileName), (std::ios_base::binary | std::ios_base::out | std::ios_base::trunc));
    if (!ofile.is_open())
    {
        return qx_bool(false, 0, QStringLiteral("Cannot create or open compressed file '%1'").arg(sFileName));
    }
    ofile.write(compressed.constData(), compressed.size());
    ofile.close();

    return true;
}

template <class T, class ArchiveInput /* = QX_DEFAULT_ARCHIVE_INPUT */, class ArchiveOutput /* = QX_DEFAULT_ARCHIVE_OUTPUT */>
qx_bool QxArchive_FromFileCompressed<T, ArchiveInput, ArchiveOutput>::fromFileCompressed(T& obj, const QString& sFileName, unsigned int flags /* = boost::archive::no_header */)
{
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_ifstream qx_type_ifstream;
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_char qx_type_char;
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_string qx_type_string;
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_istringstream qx_type_istringstream;

    qx_type_ifstream ifile(qPrintable(sFileName), (std::ios_base::binary | std::ios_base::in));
    if (!ifile.is_open())
    {
        return qx_bool(false, 0, QLatin1String("Cannot open compressed file '%1'").arg(sFileName));
    }
    ifile.seekg(0, std::ios::end);
    int iSize = ifile.tellg();
    ifile.seekg(0, std::ios::beg);
    qx_type_char* buffer = new qx_type_char[iSize];
    ifile.read(buffer, iSize);
    ifile.close();

    QByteArray uncompressed = qUncompress((unsigned char*)(buffer), iSize);
    if (uncompressed.isEmpty())
    {
        delete[] buffer; qAssert(false); 
        return qx_bool(false, 0, QStringLiteral("Cannot uncompress archive"));
    }
    delete[] buffer;

    qx_type_string str(uncompressed.constData(), uncompressed.count());
    qx_type_istringstream iss(str, (std::ios_base::binary | std::ios_base::in));
    ArchiveInput iar(iss, flags);
    qx_bool bDeserializeOk = false;
    const char* sTag = QxClassName<T>::get_xml_tag();
    QxBoostSerializeRegisterHelperX::helper(iar);

    try
    {
        iar >> boost::serialization::make_nvp(sTag, obj); bDeserializeOk = true;
    }
    catch (const boost::archive::archive_exception& e)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (const std::exception& e)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (...)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QStringLiteral("unknown error")));
    }

    if (!bDeserializeOk.getDesc().isEmpty())
    {
        QString sMsg = QStringLiteral("qx::QxArchive_FromFileCompressed<T>::fromFileCompressed() -> ") + bDeserializeOk.getDesc();
        xwarn(qPrintable(sMsg));
    }
    qAssert(bDeserializeOk);

    return bDeserializeOk;
}

template <class T, class ArchiveInput /* = QX_DEFAULT_ARCHIVE_INPUT */, class ArchiveOutput /* = QX_DEFAULT_ARCHIVE_OUTPUT */>
QString QxArchive_ToString<T, ArchiveInput, ArchiveOutput>::toString(const T& obj, unsigned int flags /* = boost::archive::no_header */)
{
    typedef typename qx::trait::archive_wide_traits<ArchiveOutput>::type_ostringstream qx_type_ostringstream;
    qx_type_ostringstream oss(std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    ArchiveOutput oar(oss, flags);
    qx_bool bSerializeOk = false;
    const char* sTag = QxClassName<T>::get_xml_tag();
    QxBoostSerializeRegisterHelperX::helper(oar);

    try
    {
        oar << boost::serialization::make_nvp(sTag, obj); bSerializeOk = oss.good();
    }
    catch (const boost::archive::archive_exception& e)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (const std::exception& e)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (...)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QStringLiteral("unknown error")));
    }

    if (!bSerializeOk.getDesc().isEmpty())
    {
        QString sMsg = QStringLiteral("qx::QxArchive_ToString<T>::toString() -> ") + bSerializeOk.getDesc(); 
        xwarn(qPrintable(sMsg));
    }
    qAssert(bSerializeOk);

    return (bSerializeOk ? qx::trait::archive_wide_traits<ArchiveOutput>::toQString(oss.str()) : QString());
}

template <class T, class ArchiveInput /* = QX_DEFAULT_ARCHIVE_INPUT */, class ArchiveOutput /* = QX_DEFAULT_ARCHIVE_OUTPUT */>
qx_bool QxArchive_FromString<T, ArchiveInput, ArchiveOutput>::fromString(T& obj, const QString& sString, unsigned int flags /* = boost::archive::no_header */)
{
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_string qx_type_string;
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_istringstream qx_type_istringstream;

    qx_type_string str;
    if (sString.isEmpty())
    {
        return false;
    }
    qx::trait::archive_wide_traits<ArchiveInput>::fromQString(sString, str);
    qx_type_istringstream iss(str, (std::ios_base::binary | std::ios_base::in));
    ArchiveInput iar(iss, flags);
    qx_bool bDeserializeOk = false;
    const char* sTag = QxClassName<T>::get_xml_tag();
    QxBoostSerializeRegisterHelperX::helper(iar);

    try
    {
        iar >> boost::serialization::make_nvp(sTag, obj); bDeserializeOk = true;
    }
    catch (const boost::archive::archive_exception& e)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (const std::exception& e)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (...)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QStringLiteral("unknown error")));
    }

    if (!bDeserializeOk.getDesc().isEmpty())
    {
        QString sMsg = QStringLiteral("qx::QxArchive_FromString<T>::fromString() -> ") + bDeserializeOk.getDesc();
        xwarn(qPrintable(sMsg));
    }
    qAssert(bDeserializeOk);

    return bDeserializeOk;
}

template <class T, class ArchiveInput /* = QX_DEFAULT_ARCHIVE_INPUT */, class ArchiveOutput /* = QX_DEFAULT_ARCHIVE_OUTPUT */>
QByteArray QxArchive_ToByteArray<T, ArchiveInput, ArchiveOutput>::toByteArray(const T& obj, type_string* owner /* = NULL */, unsigned int flags /* = boost::archive::no_header */)
{
    typedef typename qx::trait::archive_wide_traits<ArchiveOutput>::type_ostringstream qx_type_ostringstream;
    qx_type_ostringstream oss(std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    ArchiveOutput oar(oss, flags);
    qx_bool bSerializeOk = false;
    const char* sTag = QxClassName<T>::get_xml_tag();
    QxBoostSerializeRegisterHelperX::helper(oar);

    try
    {
        oar << boost::serialization::make_nvp(sTag, obj); bSerializeOk = oss.good();
    }
    catch (const boost::archive::archive_exception& e)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (const std::exception& e)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (...)
    {
        bSerializeOk.setDesc(QX_STR_SERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QStringLiteral("unknown error")));
    }

    if (!bSerializeOk.getDesc().isEmpty())
    {
        QString sMsg = QStringLiteral("qx::QxArchive_ToByteArray<T>::toByteArray() -> ") + bSerializeOk.getDesc();
        xwarn(qPrintable(sMsg));
    }
    qAssert(bSerializeOk);

    return (bSerializeOk ? qx::trait::archive_wide_traits<ArchiveOutput>::toQByteArray(oss.str(), owner) : QByteArray());
}

template <class T, class ArchiveInput /* = QX_DEFAULT_ARCHIVE_INPUT */, class ArchiveOutput /* = QX_DEFAULT_ARCHIVE_OUTPUT */>
qx_bool QxArchive_FromByteArray<T, ArchiveInput, ArchiveOutput>::fromByteArray(T& obj, const QByteArray& data, unsigned int flags /* = boost::archive::no_header */)
{
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_string qx_type_string;
    typedef typename qx::trait::archive_wide_traits<ArchiveInput>::type_istringstream qx_type_istringstream;

    qx_type_string str;
    if (data.size() <= 0)
    {
        return false;
    }
    qx::trait::archive_wide_traits<ArchiveInput>::fromQByteArray(data, str);
    qx_type_istringstream iss(str, (std::ios_base::binary | std::ios_base::in));
    ArchiveInput iar(iss, flags);
    qx_bool bDeserializeOk = false;
    const char* sTag = QxClassName<T>::get_xml_tag();
    QxBoostSerializeRegisterHelperX::helper(iar);

    try
    {
        iar >> boost::serialization::make_nvp(sTag, obj); bDeserializeOk = true;
    }
    catch (const boost::archive::archive_exception& e)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (const std::exception& e)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QLatin1String(e.what())));
    }
    catch (...)
    {
        bDeserializeOk.setDesc(QX_STR_DESERIALIZATION_ERROR.replace(QStringLiteral("%ERR%"), QStringLiteral("unknown error")));
    }

    if (!bDeserializeOk.getDesc().isEmpty())
    {
        QString sMsg = QStringLiteral("qx::QxArchive_FromByteArray<T>::fromByteArray() -> ") + bDeserializeOk.getDesc();
        xwarn(qPrintable(sMsg));
    }
    qAssert(bDeserializeOk);

    return bDeserializeOk;
}

} // namespace qx

#endif // _QX_ENABLE_BOOST_SERIALIZATION
