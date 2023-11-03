// SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractListModel>

class SavedCommandsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit SavedCommandsModel(QObject *parent = nullptr);
    ~SavedCommandsModel() override;
    
    static SavedCommandsModel* self(){
        static SavedCommandsModel *singleton = new SavedCommandsModel();
        return singleton;
    };
    
    int rowCount(const QModelIndex &parent) const override
    {
        return parent.isValid() ? 0 : m_actions.size();
    };
    
    QVariant data(const QModelIndex &index, int role) const override;
    void save();
    
    Q_INVOKABLE void addAction(const QString &action);
    Q_INVOKABLE bool removeRow(int row);

private:
    QStringList m_actions;
};
