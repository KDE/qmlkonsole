// SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QAbstractListModel>

class QuickActionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    static QuickActionModel* self(){
        static QuickActionModel singleton;
        return &singleton;
    };
    int rowCount(const QModelIndex &parent) const override
    {
        return m_actions.size();
    };
    QVariant data(const QModelIndex &index, int role) const override
    {
        return m_actions.at(index.row());
    };
    void save();
    Q_INVOKABLE void addAction(QString action);
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    QuickActionModel(QObject *parent = nullptr);

    QStringList m_actions;
};
