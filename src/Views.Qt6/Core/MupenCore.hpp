#pragma once

#include <QObject>
#include <QUrl>
#include <qqmlintegration.h>

class MupenCore : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
  public:
    MupenCore(QObject* parent = nullptr);
    virtual ~MupenCore();

    Q_INVOKABLE void vrStartROM(const QUrl& url);

    Q_INVOKABLE void vrCloseROM(bool resetVCR = true);
};