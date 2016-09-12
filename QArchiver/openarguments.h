#ifndef OPENARGUMENTS_H
#define OPENARGUMENTS_H

#include <QObject>
#include <QMap>

class OpenArguments
{
    QMap<QString, QString> m_metaData;
public:
    OpenArguments() {}

    QMap<QString, QString> &metaData()
    {
        return m_metaData;
    }

    const QMap<QString, QString> &metaData() const
    {
        return m_metaData;
    }

    void setOption(const QString &key, const QString &value)
    {
        m_metaData[key] = value;
    }
    void removeOption(const QString &key)
    {
        m_metaData.remove(key);
    }
    const QString getOption(const QString &key) const
    {
        return m_metaData.value(key);
    }
};

#endif // OPENARGUMENTS_H
