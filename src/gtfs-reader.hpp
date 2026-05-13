#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>


class GTFSReader : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static GTFSReader *create(QQmlEngine *, QJSEngine *);

    Q_INVOKABLE void request() const;

private:
    GTFSReader();
};