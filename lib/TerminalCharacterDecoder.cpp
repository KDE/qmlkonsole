/*
    This file is part of Konsole, an X terminal.

    SPDX-FileCopyrightText: 2006-2008 Robert Knight <robertknight@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

// Own
#include "TerminalCharacterDecoder.h"

// stdlib
#include <cwctype>

// Qt
#include <QTextStream>

// KDE
// #include <kdebug.h>

// Konsole
#include "konsole_wcwidth.h"

#include <cwctype>

using namespace Konsole;
PlainTextDecoder::PlainTextDecoder()
    : _output(nullptr)
    , _includeTrailingWhitespace(true)
    , _recordLinePositions(false)
{
}
void PlainTextDecoder::setTrailingWhitespace(bool enable)
{
    _includeTrailingWhitespace = enable;
}
bool PlainTextDecoder::trailingWhitespace() const
{
    return _includeTrailingWhitespace;
}
void PlainTextDecoder::begin(QTextStream *output)
{
    _output = output;
    if (!_linePositions.isEmpty())
        _linePositions.clear();
}
void PlainTextDecoder::end()
{
    _output = nullptr;
}

void PlainTextDecoder::setRecordLinePositions(bool record)
{
    _recordLinePositions = record;
}
QList<int> PlainTextDecoder::linePositions() const
{
    return _linePositions;
}

void PlainTextDecoder::decodeLine(std::span<const Character> characters, LineProperty /*properties*/)
{
    Q_ASSERT(_output);
    int count = characters.size();

    if (_recordLinePositions && _output->string()) {
        int pos = _output->string()->size();
        _linePositions << pos;
    }

    // check the real length
    for (int i = 0; i < count; i++) {
        if (characters.subspan(i).empty()) {
            count = i;
            break;
        }
    }

    // TODO should we ignore or respect the LINE_WRAPPED line property?

    // note:  we build up a QString and send it to the text stream rather writing into the text
    // stream a character at a time because it is more efficient.
    //(since QTextStream always deals with QStrings internally anyway)
    QString plainText;
    plainText.reserve(count);

    int outputCount = count;

    // if inclusion of trailing whitespace is disabled then find the end of the
    // line
    if (!_includeTrailingWhitespace) {
        for (int i = count - 1; i >= 0; i--) {
            if (characters[i].character != u' ')
                break;
            else
                outputCount--;
        }
    }

    for (int i = 0; i < outputCount;) {
        plainText.push_back(characters[i].character);
        i += qMax(1, konsole_wcwidth(characters[i].character));
    }
    *_output << plainText;
}
