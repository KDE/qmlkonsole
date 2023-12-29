/*
    This file is part of Konsole, an X terminal.

    SPDX-FileCopyrightText: 2007-2008 Robert Knight <robertknight@gmail.com>
    SPDX-FileCopyrightText: 1997, 1998 Lars Doelle <lars.doelle@on-line.de>

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

#ifndef VT102EMULATION_H
#define VT102EMULATION_H

// Standard Library
#include <cstdio>

// Qt
#include <QHash>
#include <QKeyEvent>
#include <QTimer>

// Konsole
#include "Emulation.h"
#include "Screen.h"

constexpr auto MODE_AppScreen = (MODES_SCREEN + 0); // Mode #1
constexpr auto MODE_AppCuKeys = (MODES_SCREEN + 1); // Application cursor keys (DECCKM)
constexpr auto MODE_AppKeyPad = (MODES_SCREEN + 2); //
constexpr auto MODE_Mouse1000 = (MODES_SCREEN + 3); // Send mouse X,Y position on press and release
constexpr auto MODE_Mouse1001 = (MODES_SCREEN + 4); // Use Hilight mouse tracking
constexpr auto MODE_Mouse1002 = (MODES_SCREEN + 5); // Use cell motion mouse tracking
constexpr auto MODE_Mouse1003 = (MODES_SCREEN + 6); // Use all motion mouse tracking
constexpr auto MODE_Mouse1005 = (MODES_SCREEN + 7); // Xterm-style extended coordinates
constexpr auto MODE_Mouse1006 = (MODES_SCREEN + 8); // 2nd Xterm-style extended coordinates
constexpr auto MODE_Mouse1015 = (MODES_SCREEN + 9); // Urxvt-style extended coordinates
constexpr auto MODE_Ansi = (MODES_SCREEN + 10); // Use US Ascii for character sets G0-G3 (DECANM)
constexpr auto MODE_132Columns = (MODES_SCREEN + 11); // 80 <-> 132 column mode switch (DECCOLM)
constexpr auto MODE_Allow132Columns = (MODES_SCREEN + 12); // Allow DECCOLM mode
constexpr auto MODE_BracketedPaste = (MODES_SCREEN + 13); // Xterm-style bracketed paste mode
constexpr auto MODE_total = (MODES_SCREEN + 14);

namespace Konsole
{

struct CharCodes {
    // coding info
    char charset[4]; //
    int cu_cs; // actual charset.
    bool graphic; // Some VT100 tricks
    bool pound; // Some VT100 tricks
    bool sa_graphic; // saved graphic
    bool sa_pound; // saved pound
};

/**
 * Provides an xterm compatible terminal emulation based on the DEC VT102 terminal.
 * A full description of this terminal can be found at http://vt100.net/docs/vt102-ug/
 *
 * In addition, various additional xterm escape sequences are supported to provide
 * features such as mouse input handling.
 * See http://rtfm.etla.org/xterm/ctlseq.html for a description of xterm's escape
 * sequences.
 *
 */
class Vt102Emulation : public Emulation
{
    Q_OBJECT

public:
    /** Constructs a new emulation */
    Vt102Emulation();
    ~Vt102Emulation() override;

    // reimplemented from Emulation
    void clearEntireScreen() override;
    void reset() override;
    char eraseChar() const override;

public Q_SLOTS:
    // reimplemented from Emulation
    void sendString(const char *, int length = -1) override;
    void sendText(const QString &text) override;
    void sendKeyEvent(QKeyEvent *, bool fromPaste) override;
    void sendMouseEvent(int buttons, int column, int line, int eventType) override;
    virtual void focusLost();
    virtual void focusGained();

protected:
    // reimplemented from Emulation
    void setMode(int mode) override;
    void resetMode(int mode) override;
    void receiveChar(QChar cc) override;

private Q_SLOTS:
    // causes changeTitle() to be emitted for each (int,QString) pair in pendingTitleUpdates
    // used to buffer multiple title updates
    void updateTitle();

private:
    wchar_t applyCharset(char16_t c);
    void setCharset(int n, int cs);
    void useCharset(int n);
    void setAndUseCharset(int n, int cs);
    void saveCursor();
    void restoreCursor();
    void resetCharset(int scrno);
    QKeyEvent *remapKeyModifiersForMac(QKeyEvent *event);

    void setMargins(int top, int bottom);
    // set margins for all screens back to their defaults
    void setDefaultMargins();

    // returns true if 'mode' is set or false otherwise
    bool getMode(int mode);
    // saves the current boolean value of 'mode'
    void saveMode(int mode);
    // restores the boolean value of 'mode'
    void restoreMode(int mode);
    // resets all modes
    // (except MODE_Allow132Columns)
    void resetModes();

    void resetTokenizer();
#define MAX_TOKEN_LENGTH 256 // Max length of tokens (e.g. window title)
    void addToCurrentToken(char16_t cc);
    char16_t tokenBuffer[MAX_TOKEN_LENGTH]; // FIXME: overflow?
    int tokenBufferPos;
#define MAXARGS 15
    void addDigit(int dig);
    void addArgument();
    int argv[MAXARGS];
    int argc;
    void initTokenizer();
    int prevCC;

    // Set of flags for each of the ASCII characters which indicates
    // what category they fall into (printable character, control, digit etc.)
    // for the purposes of decoding terminal output
    int charClass[256];

    void reportDecodingError();

    void processToken(int code, char16_t p, int q);
    void processWindowAttributeChange();
    void requestWindowAttribute(int);

    void reportTerminalType();
    void reportSecondaryAttributes();
    void reportStatus();
    void reportAnswerBack();
    void reportCursorPosition();
    void reportTerminalParms(int p);

    void onScrollLock();
    void scrollLock(const bool lock);

    // clears the screen and resizes it to the specified
    // number of columns
    void clearScreenAndSetColumns(int columnCount);

    CharCodes _charset[2];

    class TerminalState
    {
    public:
        // Initializes all modes to false
        TerminalState()
        {
            memset(&mode, false, MODE_total * sizeof(bool));
        }

        bool mode[MODE_total];
    };

    TerminalState _currentModes;
    TerminalState _savedModes;

    // hash table and timer for buffering calls to the session instance
    // to update the name of the session
    // or window title.
    // these calls occur when certain escape sequences are seen in the
    // output from the terminal
    QHash<int, QString> _pendingTitleUpdates;
    QTimer *_titleUpdateTimer;

    bool _reportFocusEvents;
};

}

#endif // VT102EMULATION_H
