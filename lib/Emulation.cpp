/*
    SPDX-FileCopyrightText: 2007-2008 Robert Knight <robertknight@gmail.com>
    SPDX-FileCopyrightText: 1997, 1998 Lars Doelle <lars.doelle@on-line.de>
    SPDX-FileCopyrightText: 1996 Matthias Ettrich <ettrich@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

// Own
#include "Emulation.h"

// System
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

// Qt
#include <QApplication>
#include <QClipboard>
#include <QHash>
#include <QKeyEvent>
#include <QRegExp>
#include <QTextStream>
#include <QThread>

#include <QTime>

// KDE
// #include <kdebug.h>

// Konsole
#include "KeyboardTranslator.h"
#include "Screen.h"
#include "ScreenWindow.h"
#include "TerminalCharacterDecoder.h"

using namespace Konsole;

Emulation::Emulation()
    : _currentScreen(nullptr)
    , _codec(nullptr)
    , _decoder(nullptr)
    , _keyTranslator(nullptr)
    , _usesMouse(false)
    , _bracketedPasteMode(false)
{
    // create screens with a default size
    _primaryScreen = std::make_unique<Screen>(40, 80);
    _alternateScreen = std::make_unique<Screen>(40, 80);
    _currentScreen = _primaryScreen.get();

    QObject::connect(&_bulkTimer1, &QTimer::timeout, this, &Emulation::showBulk);
    QObject::connect(&_bulkTimer2, &QTimer::timeout, this, &Emulation::showBulk);

    // listen for mouse status changes
    connect(this, &Emulation::programUsesMouseChanged, &Emulation::usesMouseChanged);
    connect(this, &Emulation::programBracketedPasteModeChanged, &Emulation::bracketedPasteModeChanged);

    connect(this, &Emulation::cursorChanged, [this](KeyboardCursorShape cursorShape, bool blinkingCursorEnabled) {
        Q_EMIT titleChanged(50,
                            QString(QLatin1String("CursorShape=%1;BlinkingCursorEnabled=%2")).arg(static_cast<int>(cursorShape)).arg(blinkingCursorEnabled));
    });
}

Emulation::~Emulation() = default;

bool Emulation::programUsesMouse() const
{
    return _usesMouse;
}

void Emulation::usesMouseChanged(bool usesMouse)
{
    _usesMouse = usesMouse;
}

bool Emulation::programBracketedPasteMode() const
{
    return _bracketedPasteMode;
}

void Emulation::bracketedPasteModeChanged(bool bracketedPasteMode)
{
    _bracketedPasteMode = bracketedPasteMode;
}

ScreenWindow *Emulation::createWindow()
{
    auto window = std::make_unique<ScreenWindow>();
    window->setScreen(_currentScreen);

    connect(window.get(), &ScreenWindow::selectionChanged, this, &Emulation::bufferedUpdate);

    connect(this, &Emulation::outputChanged, window.get(), &ScreenWindow::notifyOutputChanged);

    connect(this, &Emulation::handleCommandFromKeyboard, window.get(), &ScreenWindow::handleCommandFromKeyboard);
    connect(this, &Emulation::outputFromKeypressEvent, window.get(), &ScreenWindow::scrollToEnd);

    _windows.push_back(std::move(window));

    return _windows.back().get();
}

void Emulation::setScreen(int n)
{
    Screen *old = _currentScreen;
    Screen *screens[2] = {_primaryScreen.get(), _alternateScreen.get()};
    _currentScreen = screens[n & 1];
    if (_currentScreen != old) {
        // tell all windows onto this emulation to switch to the newly active screen
        for (const auto &window : std::as_const(_windows))
            window->setScreen(_currentScreen);
    }
}

void Emulation::clearHistory()
{
    _primaryScreen->setScroll(_primaryScreen->getScroll(), false);
}
void Emulation::setHistory(const HistoryType &t)
{
    _primaryScreen->setScroll(t);

    showBulk();
}

const HistoryType &Emulation::history() const
{
    return _primaryScreen->getScroll();
}

void Emulation::setCodec(const QTextCodec *qtc)
{
    if (qtc)
        _codec = qtc;
    else
        setCodec(LocaleCodec);

    _decoder.reset();
    _decoder = std::unique_ptr<QTextDecoder>(_codec->makeDecoder());

    Q_EMIT useUtf8Request(utf8());
}

void Emulation::setCodec(EmulationCodec codec)
{
    if (codec == Utf8Codec)
        setCodec(QTextCodec::codecForName("utf8"));
    else if (codec == LocaleCodec)
        setCodec(QTextCodec::codecForLocale());
}

void Emulation::setKeyBindings(const QString &name)
{
    _keyTranslator = KeyboardTranslatorManager::instance()->findTranslator(name);
    if (!_keyTranslator) {
        _keyTranslator = KeyboardTranslatorManager::instance()->defaultTranslator();
    }
}

QString Emulation::keyBindings() const
{
    return _keyTranslator->name();
}

void Emulation::receiveChar(QChar c)
// process application unicode input to terminal
// this is a trivial scanner
{
    c.unicode() &= 0xff;
    switch (c.unicode()) {
    case '\b':
        _currentScreen->backspace();
        break;
    case '\t':
        _currentScreen->tab();
        break;
    case '\n':
        _currentScreen->newLine();
        break;
    case '\r':
        _currentScreen->toStartOfLine();
        break;
    case 0x07:
        Q_EMIT stateSet(NOTIFYBELL);
        break;
    default:
        _currentScreen->displayCharacter(c);
        break;
    };
}

void Emulation::sendKeyEvent(QKeyEvent *ev, bool)
{
    Q_EMIT stateSet(NOTIFYNORMAL);

    if (!ev->text().isEmpty()) { // A block of text
        // Note that the text is proper unicode.
        // We should do a conversion here
        Q_EMIT sendData(ev->text().toUtf8().constData(), ev->text().length());
    }
}

void Emulation::sendString(const char *, int)
{
    // default implementation does nothing
}

void Emulation::sendMouseEvent(int /*buttons*/, int /*column*/, int /*row*/, int /*eventType*/)
{
    // default implementation does nothing
}

/*
   We are doing code conversion from locale to unicode first.
TODO: Character composition from the old code.  See #96536
*/

void Emulation::receiveData(const char *text, int length)
{
    Q_EMIT stateSet(NOTIFYACTIVITY);

    bufferedUpdate();

    /* XXX: the following code involves encoding & decoding of "UTF-16
     * surrogate pairs", which does not work with characters higher than
     * U+10FFFF
     * https://unicodebook.readthedocs.io/unicode_encodings.html#surrogates
     */
    QString utf16Text = _decoder->toUnicode(text, length);

    // send characters to terminal emulator
    for (auto c : utf16Text) {
        receiveChar(c);
    }

    // look for z-modem indicator
    //-- someone who understands more about z-modems that I do may be able to move
    // this check into the above for loop?
    for (int i = 0; i < length; i++) {
        if (text[i] == '\030') {
            if ((length - i - 1 > 3) && (strncmp(text + i + 1, "B00", 3) == 0))
                Q_EMIT zmodemDetected();
        }
    }
}

// OLDER VERSION
// This version of onRcvBlock was commented out because
//     a)  It decoded incoming characters one-by-one, which is slow in the current version of Qt (4.2 tech preview)
//     b)  It messed up decoding of non-ASCII characters, with the result that (for example) chinese characters
//         were not printed properly.
//
// There is something about stopping the _decoder if "we get a control code halfway a multi-byte sequence" (see below)
// which hasn't been ported into the newer function (above).  Hopefully someone who understands this better
// can find an alternative way of handling the check.

/*void Emulation::onRcvBlock(const char *s, int len)
{
  Q_EMIT notifySessionState(NOTIFYACTIVITY);

  bufferedUpdate();
  for (int i = 0; i < len; i++)
  {

    QString result = _decoder->toUnicode(&s[i],1);
    int reslen = result.length();

    // If we get a control code halfway a multi-byte sequence
    // we flush the _decoder and continue with the control code.
    if ((s[i] < 32) && (s[i] > 0))
    {
       // Flush _decoder
       while(!result.length())
          result = _decoder->toUnicode(&s[i],1);
       reslen = 1;
       result.resize(reslen);
       result[0] = QChar(s[i]);
    }

    for (int j = 0; j < reslen; j++)
    {
      if (result[j].characterategory() == QChar::Mark_NonSpacing)
         _currentScreen->compose(result.mid(j,1));
      else
         onRcvChar(result[j].unicode());
    }
    if (s[i] == '\030')
    {
      if ((len-i-1 > 3) && (strncmp(s+i+1, "B00", 3) == 0))
          Q_EMIT zmodemDetected();
    }
  }
}*/

void Emulation::writeToStream(TerminalCharacterDecoder *_decoder, int startLine, int endLine)
{
    _currentScreen->writeLinesToStream(_decoder, startLine, endLine);
}

void Emulation::writeToStream(TerminalCharacterDecoder *_decoder)
{
    _currentScreen->writeLinesToStream(_decoder, 0, _currentScreen->getHistLines());
}

int Emulation::lineCount() const
{
    // sum number of lines currently on _screen plus number of lines in history
    return _currentScreen->getLines() + _currentScreen->getHistLines();
}

#define BULK_TIMEOUT1 10
#define BULK_TIMEOUT2 40

void Emulation::showBulk()
{
    _bulkTimer1.stop();
    _bulkTimer2.stop();

    Q_EMIT outputChanged();

    _currentScreen->resetScrolledLines();
    _currentScreen->resetDroppedLines();
}

void Emulation::bufferedUpdate()
{
    _bulkTimer1.setSingleShot(true);
    _bulkTimer1.start(BULK_TIMEOUT1);
    if (!_bulkTimer2.isActive()) {
        _bulkTimer2.setSingleShot(true);
        _bulkTimer2.start(BULK_TIMEOUT2);
    }
}

char Emulation::eraseChar() const
{
    return '\b';
}

void Emulation::setImageSize(int lines, int columns)
{
    if ((lines < 1) || (columns < 1))
        return;

    QSize screenSize[2] = {QSize(_primaryScreen->getColumns(), _primaryScreen->getLines()),
                           QSize(_alternateScreen->getColumns(), _alternateScreen->getLines())};
    QSize newSize(columns, lines);

    if (newSize == screenSize[0] && newSize == screenSize[1])
        return;

    _primaryScreen->resizeImage(lines, columns);
    _alternateScreen->resizeImage(lines, columns);

    Q_EMIT imageSizeChanged(lines, columns);

    bufferedUpdate();
}

QSize Emulation::imageSize() const
{
    return {_currentScreen->getColumns(), _currentScreen->getLines()};
}

ushort ExtendedCharTable::extendedCharHash(ushort *unicodePoints, ushort length) const
{
    ushort hash = 0;
    for (ushort i = 0; i < length; i++) {
        hash = 31 * hash + unicodePoints[i];
    }
    return hash;
}
bool ExtendedCharTable::extendedCharMatch(ushort hash, ushort *unicodePoints, ushort length) const
{
    std::span entry = extendedCharTable.at(hash);

    // compare given length with stored sequence length ( given as the first ushort in the
    // stored buffer )
    if (entry.empty() || entry[0] != length)
        return false;
    // if the lengths match, each character must be checked.  the stored buffer starts at
    // entry[1]
    for (int i = 0; i < length; i++) {
        if (entry[i + 1] != unicodePoints[i])
            return false;
    }
    return true;
}
ushort ExtendedCharTable::createExtendedChar(ushort *unicodePoints, ushort length)
{
    // look for this sequence of points in the table
    ushort hash = extendedCharHash(unicodePoints, length);

    // check existing entry for match
    while (extendedCharTable.contains(hash)) {
        if (extendedCharMatch(hash, unicodePoints, length)) {
            // this sequence already has an entry in the table,
            // return its hash
            return hash;
        } else {
            // if hash is already used by another, different sequence of unicode character
            // points then try next hash
            hash++;
        }
    }

    // add the new sequence to the table and
    // return that index
    std::vector<ushort> buffer(length + 1);
    buffer[0] = length;
    for (int i = 0; i < length; i++)
        buffer[i + 1] = unicodePoints[i];

    extendedCharTable.insert({hash, std::move(buffer)});

    return hash;
}

std::span<const ushort> ExtendedCharTable::lookupExtendedChar(ushort hash, ushort &length) const
{
    // lookup index in table and if found, set the length
    // argument and return a pointer to the character sequence

    std::span buffer = extendedCharTable.at(hash);
    if (!buffer.empty()) {
        length = buffer[0];
        return buffer.subspan(1);
    } else {
        length = 0;
        return std::span<ushort>();
    }
}

ExtendedCharTable::ExtendedCharTable() = default;
ExtendedCharTable::~ExtendedCharTable() = default;

// global instance
ExtendedCharTable ExtendedCharTable::instance;

// #include "Emulation.moc"
