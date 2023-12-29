// SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "terminaltabmodel.h"

#include <KLocalizedString>
#include <QDebug>

TerminalTabModel::TerminalTabModel(QObject *parent)
    : QAbstractListModel{parent}
    , m_indexCounter{1}
    , m_tabNames{}
{
    newTab(); // default tab
}

int TerminalTabModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_tabNames.count();
}

QVariant TerminalTabModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tabNames.count()) {
        return {};
    }

    if (role == NameRole) {
        return m_tabNames[index.row()];
    } else {
        return {};
    }
}

bool TerminalTabModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_tabNames.count()) {
        return false;
    }

    if (role == NameRole) {
        m_tabNames[index.row()] = value.toString();
    } else {
        return false;
    }

    Q_EMIT dataChanged(index, index);
    return true;
}

Qt::ItemFlags TerminalTabModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEditable;
}

QHash<int, QByteArray> TerminalTabModel::roleNames() const
{
    return {{NameRole, "name"}};
}

Q_INVOKABLE void TerminalTabModel::newTab()
{
    auto index = m_indexCounter;

    Q_EMIT beginInsertRows(QModelIndex(), m_tabNames.count(), m_tabNames.count());
    m_tabNames.append(i18n("Tab %1", index));
    Q_EMIT endInsertRows();

    ++m_indexCounter;
}

void TerminalTabModel::removeTab(int index)
{
    if (index < 0 || index >= m_tabNames.count()) {
        return;
    }

    Q_EMIT beginRemoveRows(QModelIndex(), index, index);
    m_tabNames.removeAt(index);
    Q_EMIT endRemoveRows();

    if (m_tabNames.count() == 0) {
        m_indexCounter = 1;
        newTab();
    }
}
