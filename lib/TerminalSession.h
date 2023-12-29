/*
    This file is part of Konsole QML plugin,
    which is a terminal emulator from KDE.

    SPDX-FileCopyrightText: 2013 Dmitry Zagnoyko <hiroshidi@gmail.com>

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

#ifndef KSESSION_H
#define KSESSION_H

#include <QObject>

// Konsole
#include "Session.h"

using namespace Konsole;

class TerminalSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString  kbScheme  READ  getKeyBindings WRITE setKeyBindings NOTIFY changedKeyBindings)
    Q_PROPERTY(QString  initialWorkingDirectory READ getInitialWorkingDirectory WRITE setInitialWorkingDirectory NOTIFY initialWorkingDirectoryChanged)
    Q_PROPERTY(QString  title READ getTitle WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString  shellProgram READ shellProgram WRITE setShellProgram NOTIFY shellProgramChanged)
    Q_PROPERTY(QStringList shellProgramArgs READ args WRITE setArgs NOTIFY argsChanged)
    Q_PROPERTY(QString  history READ getHistory)
    Q_PROPERTY(bool hasActiveProcess READ hasActiveProcess)
    Q_PROPERTY(QString foregroundProcessName READ foregroundProcessName)
    Q_PROPERTY(QString currentDir READ currentDir)

public:
    TerminalSession(QObject *parent = nullptr);
    ~TerminalSession() override ;

public:
    //bool setup();
    void addView(TerminalDisplay *display);
    void removeView(TerminalDisplay *display);

    int getRandomSeed();
    QString getKeyBindings();

    //look-n-feel, if you don`t like defaults

    //environment
    void setEnvironment(const QStringList & environment);

    //Initial working directory
    void setInitialWorkingDirectory(const QString & dir);
    QString getInitialWorkingDirectory();

    //Text codec, default is UTF-8
    void setTextCodec(QTextCodec * codec);

    // History size for scrolling
    void setHistorySize(int lines); //infinite if lines < 0
    int historySize() const;

    QString getHistory() const;

    // Sets whether flow control is enabled
    void setFlowControlEnabled(bool enabled);

    // Returns whether flow control is enabled
    bool flowControlEnabled(void);

    /**
     * Sets whether the flow control warning box should be shown
     * when the flow control stop key (Ctrl+S) is pressed.
     */
    //void setFlowControlWarningEnabled(bool enabled);

    /*! Get all available keyboard bindings
     */
    static QStringList availableKeyBindings();

    //! Return current key bindings
    QString keyBindings();

    QString getTitle();

    /**
     * Returns \c true if the session has an active subprocess running in it
     * spawned from the initial shell.
     */
    bool hasActiveProcess() const;

    /**
     * Returns the name of the terminal's foreground process.
     */
    QString foregroundProcessName();

    /**
     * Returns the current working directory of the process.
     */
    QString currentDir();

Q_SIGNALS:
    void started();
    void finished();
    void copyAvailable(bool);

    void termGetFocus();
    void termLostFocus();

    void termKeyPressed(QKeyEvent *, bool);

    void changedKeyBindings(QString kb);

    void titleChanged();

    void historySizeChanged();

    void initialWorkingDirectoryChanged();

    void matchFound(int startColumn, int startLine, int endColumn, int endLine);
    void noMatchFound();

    void shellProgramChanged();
    void argsChanged();

public Q_SLOTS:
    /*! Set named key binding for given widget
     */
    void setKeyBindings(const QString & kb);
    void setTitle(QString name);

    void startShellProgram();

    bool sendSignal(int signal);

    //  Shell program, default is /bin/bash
    QString shellProgram() const;
    void setShellProgram(const QString & progname);

    // Shell program args, default is none
    QStringList args() const;
    void setArgs(const QStringList &args);

    int getShellPID();
    void changeDir(const QString & dir);

    // Send some text to terminal
    void sendText(QString text);
    // Send some text to terminal
    void sendKey(int rep, int key, int mod) const;

    void clearScreen();

    // Search history
    void search(const QString &regexp, int startLine = 0, int startColumn = 0, bool forwards = true );

    void selectionChanged(bool textSelected);

protected Q_SLOTS:
    void sessionFinished();

private Q_SLOTS:
    std::unique_ptr<Session> createSession(QString name);

private:
    QString _initialWorkingDirectory;
    std::unique_ptr<Session> m_session;
};

#endif // KSESSION_H
