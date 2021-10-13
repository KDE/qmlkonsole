/*
 * Copyright 2021 Devin Lin <devin@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QAbstractListModel>
#include <QObject>

class TerminalTabModel : public QAbstractListModel
{
    Q_OBJECT
    
public:
    TerminalTabModel(QObject *parent = nullptr);
    
    static TerminalTabModel *self()
    {
        static TerminalTabModel *singleton = new TerminalTabModel();
        return singleton;
    }
    
    enum {
        NameRole = Qt::DisplayRole
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void newTab();
    Q_INVOKABLE void removeTab(int index);

private:
    int m_indexCounter;
    QList<QString> m_tabNames;
};
