#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2020 Yuri Chornoivan
# SPDX-License-Identifier: GPL-2.0-or-later

$XGETTEXT `find -name \*.cpp -o -name \*.qml -o -name \*.js` -o $podir/qmlkonsole.pot

