/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <memory>
#include <vector>
#include <version>

#include <QObject>
#include <QAbstractTableModel>
#include <QQmlListProperty>
#include <qqmlintegration.h>

#include <Core/API.hpp>

class MovieMetadata : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString author READ author NOTIFY authorChanged)
    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
    Q_PROPERTY(int startFlags READ startFlags NOTIFY startFlagsChanged)
  public:
    MovieMetadata(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isLoading() const { return m_loading; }
    QString author() const { return m_author; }
    QString description() const { return m_description; }
    int startFlags() const { return m_startFlags; }

    Q_INVOKABLE void loadMovie(const QString& path);

  signals:
    void loadingChanged();
    void authorChanged();
    void descriptionChanged();
    void startFlagsChanged();

    void valuesUpdated();

  private:
    using ExtraDataList = QList<std::pair<QString, QString>>;

    static ExtraDataList extractExtraData(CoreCtx* core_ctx, const CoreVCRMovieHeader& header, std::span<CoreButtons> inputs);

    // QML should never change this, only we do
    Q_INVOKABLE void setLoading(bool value)
    {
        if (value == m_loading) return;
        m_loading = value;
        loadingChanged();
    }

    Q_INVOKABLE void updateValues(QAnyStringView author, QAnyStringView description, int startFlags, const ExtraDataList &extraData);

    QString m_author {};
    QString m_description {};
    int m_startFlags {0};
    ExtraDataList m_extraData;

    bool m_loading;
};
