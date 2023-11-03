// SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "savedcommandsmodel.h"
#include "terminalsettings.h"

SavedCommandsModel::SavedCommandsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_actions(TerminalSettings::self()->actions())
{
}

SavedCommandsModel::~SavedCommandsModel()
{}

QVariant SavedCommandsModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)

    if (!index.isValid() || index.row() < 0 || index.row() >= m_actions.count()) {
        return false;
    }

    return m_actions.at(index.row());
}

void SavedCommandsModel::save()
{
    TerminalSettings::self()->setActions(m_actions);
}

void SavedCommandsModel::addAction(const QString &action)
{
    beginInsertRows(QModelIndex(), m_actions.size(), m_actions.size());
    m_actions.push_back(action);
    endInsertRows();
    
    TerminalSettings::self()->save();
}

bool SavedCommandsModel::removeRow(int row)
{
    if (row < 0 || row >= m_actions.size()) {
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_actions.erase(m_actions.begin() + row);
    endRemoveRows();

    TerminalSettings::self()->save();
    return true;
}
