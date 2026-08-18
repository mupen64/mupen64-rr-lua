#include "MupenCore.hpp"
#include "Core.hpp"

#include <QUrl>

MupenCore::MupenCore(QObject *parent) : QObject(parent)
{
    Core::context();
}

MupenCore::~MupenCore()
{
}

void MupenCore::vrStartROM(const QUrl &url)
{
    Core::context()->vr_start_rom(url.toLocalFile().toStdU16String());
}

void MupenCore::vrCloseROM(bool resetVCR)
{
    Core::context()->vr_close_rom(resetVCR);
}