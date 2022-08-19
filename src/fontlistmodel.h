// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class FontListModel : public QAbstractListModel {
    Q_OBJECT
    
public:
    static FontListModel *self();
    
    explicit FontListModel(QObject *parent = nullptr);
    
    enum Roles {
        NameRole = Qt::DisplayRole
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    
private:
    QStringList m_fontList;
};

class FontListSearchModel : public QSortFilterProxyModel {
    Q_OBJECT
    
public:
    static FontListSearchModel *self();
    
    explicit FontListSearchModel(QObject *parent = nullptr);
};
