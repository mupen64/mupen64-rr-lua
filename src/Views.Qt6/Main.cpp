/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "QtIconImageProvider.hpp"
#include <Common/VersionNameHelpers.hpp>
#include <Common.Views/App.hpp>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QTranslator>
#include <QtQml/QQmlExtensionPlugin>

#include <QQuickStyle>

Q_IMPORT_QML_PLUGIN(ActionsPlugin)
Q_IMPORT_QML_PLUGIN(CorePlugin)
Q_IMPORT_QML_PLUGIN(ComponentsPlugin)
Q_IMPORT_QML_PLUGIN(ConfigPlugin)
Q_IMPORT_QML_PLUGIN(UtilsPlugin)

namespace
{
using namespace Qt::Literals;
constexpr QLatin1StringView ORG_DOMAIN = "mupen64.com"_L1;
constexpr QLatin1StringView ORG_NAME = "Mupen64"_L1;
constexpr QLatin1StringView DESKTOP_FILE_NAME = "mupen64-rr-lua"_L1;
constexpr QLatin1StringView DISPLAY_NAME = "Mupen64"_L1;

} // namespace

static int qt_main(int argc, char *argv[])
{
    using namespace Qt::Literals;

    // NOTE: QApplication is used here specifically to ensure KDE's desktop styles are loaded.
    // When a QGuiApplication is used, KDE switches to its fallback Breeze theme, which
    // is slightly bugged.
    // TODO: provide and package .desktop file for Linux.
    QApplication app(argc, argv);
    QApplication::setOrganizationDomain(ORG_DOMAIN);
    QApplication::setOrganizationName(ORG_NAME);
    QApplication::setApplicationName(DESKTOP_FILE_NAME);
    QApplication::setApplicationVersion(CURRENT_VERSION);
    QApplication::setApplicationDisplayName(DISPLAY_NAME);

    // Load fallback translations first
    auto *fallbackTranslator = new QTranslator(&app);
    if (!fallbackTranslator->load("mupen64-rr_en.qm", ":/i18n"))
        throw std::runtime_error("failed to load fallback translations");
    QApplication::installTranslator(fallbackTranslator);

    // Load potential localized translations after
    auto *translator = new QTranslator(&app);
    if (translator->load(QLocale(), "mupen64-rr", "_", ":/i18n"))
        QApplication::installTranslator(translator);
    else
        delete translator;

    QQmlApplicationEngine engine;

    // Close if object creation fails
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [](const QUrl &url) {
            std::println("objectCreationFailed: {}", url.toString().toStdString());
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

#if defined(_WIN32)
    // Windows: default to Fusion, as the system theme isn't exactly nice.
    QQuickStyle::setStyle("Fusion");
#endif

    // provider for system icons
    engine.addImageProvider(u"icons"_s, new QtIconImageProvider);

    // load and run Views/MainWindow.qml
    engine.loadFromModule("Views", "MainWindow");

    return QGuiApplication::exec();
}

int main(int argc, char *argv[])
{
    return qt_main(argc, argv);
}
