#pragma once

#include <QObject>
#include <qqmlintegration.h>

class MupenCore : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
  public:
    MupenCore(QObject* parent = nullptr);
    virtual ~MupenCore();
    
  private:
    void vrStartROM(const QUrl& url);

    void vrCloseROM(bool resetVCR = true);
};