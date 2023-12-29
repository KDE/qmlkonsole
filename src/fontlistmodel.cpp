// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "fontlistmodel.h"

#include <QFontDatabase>

FontListModel *FontListModel::self()
{
    static FontListModel *singleton = new FontListModel();
    return singleton;
}

FontListModel::FontListModel(QObject *parent)
    : QAbstractListModel{parent}
{
    m_fontList = QFontDatabase::families(QFontDatabase::Any);
}

int FontListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_fontList.count();
}

QVariant FontListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_fontList.size() || index.row() < 0) {
        return {};
    }

    switch (role) {
    case NameRole:
        return m_fontList[index.row()];
    }
    return {};
}

QHash<int, QByteArray> FontListModel::roleNames() const
{
    return {{NameRole, "name"}};
}

FontListSearchModel *FontListSearchModel::self()
{
    static auto singleton = new FontListSearchModel();
    return singleton;
}

FontListSearchModel::FontListSearchModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(FontListModel::self());
    setFilterRole(FontListModel::NameRole);
}
