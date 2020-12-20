// SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#include "quickactionmodel.h"

#include "terminalsettings.h"
QuickActionModel::QuickActionModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_actions(TerminalSettings::self()->actions())
{
}

QVariant QuickActionModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)

    if (!index.isValid() || index.row() < 0 || index.row() >= m_actions.count()) {
        return false;
    }

    return m_actions.at(index.row());
}

void QuickActionModel::save()
{
    TerminalSettings::self()->setActions(m_actions);
}
void QuickActionModel::addAction(QString action)
{
    beginInsertRows(QModelIndex(), m_actions.size(), m_actions.size());
    m_actions.push_back(action);
    endInsertRows();
}

bool QuickActionModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent)

    if (row < 0 || count <= 0 || row + count - 1 >= static_cast<int>(m_actions.size()))
        return false;

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    m_actions.erase(m_actions.begin() + row, m_actions.begin() + count);
    endRemoveRows();

    return true;
}
