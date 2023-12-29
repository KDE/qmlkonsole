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

// Own
#include "TerminalSession.h"

// Qt
#include <QTextCodec>

// Konsole
#include "HistorySearch.h"
#include "KeyboardTranslator.h"

TerminalSession::TerminalSession(QObject *parent)
    : QObject(parent)
    , m_session(createSession(QString()))
{
    connect(m_session.get(), &Konsole::Session::started, this, &TerminalSession::started);
    connect(m_session.get(), &Konsole::Session::finished, this, &TerminalSession::sessionFinished);
    connect(m_session.get(), &Konsole::Session::titleChanged, this, &TerminalSession::titleChanged);
}

TerminalSession::~TerminalSession()
{
    if (m_session) {
        m_session->close();
        m_session->disconnect();
    }
}

void TerminalSession::setTitle(QString name)
{
    m_session->setTitle(Session::NameRole, name);
}

std::unique_ptr<Session> TerminalSession::createSession(QString name)
{
    auto session = std::make_unique<Session>();

    session->setTitle(Session::NameRole, name);

    /* Thats a freaking bad idea!!!!
     * /bin/bash is not there on every system
     * better set it to the current $SHELL
     * Maybe you can also make a list available and then let the widget-owner decide what to use.
     * By setting it to $SHELL right away we actually make the first filecheck obsolete.
     * But as iam not sure if you want to do anything else ill just let both checks in and set this to $SHELL anyway.
     */

    // cool-old-term: There is another check in the code. Not sure if useful.

    QString envshell = qEnvironmentVariable("SHELL");
    QString shellProg = !envshell.isNull() ? envshell : QStringLiteral("/bin/bash");
    session->setProgram(shellProg);

    setenv("TERM", "xterm", 1);

    // session->setProgram();

    QStringList args;
    session->setArguments(args);
    session->setAutoClose(true);

    session->setCodec(QTextCodec::codecForName("UTF-8"));

    session->setFlowControlEnabled(true);
    session->setHistoryType(HistoryTypeBuffer(1000));

    session->setDarkBackground(true);

    session->setKeyBindings(QString());

    return session;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

int TerminalSession::getRandomSeed()
{
    return m_session->sessionId() * 31;
}

void TerminalSession::addView(TerminalDisplay *display)
{
    m_session->setView(display);
}

void TerminalSession::removeView(TerminalDisplay *display)
{
    m_session->removeView(display);
}

void TerminalSession::sessionFinished()
{
    Q_EMIT finished();
}

void TerminalSession::selectionChanged(bool textSelected)
{
    Q_UNUSED(textSelected)
}

void TerminalSession::startShellProgram()
{
    if (m_session->isRunning()) {
        return;
    }

    m_session->run();
}

bool TerminalSession::sendSignal(int signal)
{
    if (!m_session->isRunning()) {
        return false;
    }

    return m_session->sendSignal(signal);
}

int TerminalSession::getShellPID()
{
    return m_session->processId();
}

void TerminalSession::changeDir(const QString &dir)
{
    /*
       this is a very hackish way of trying to determine if the shell is in
       the foreground before attempting to change the directory.  It may not
       be portable to anything other than Linux.
    */
    QString strCmd;
    strCmd.setNum(getShellPID());
    strCmd.prepend(u"ps -j ");
    strCmd.append(u" | tail -1 | awk '{ print $5 }' | grep -q \\+");
    int retval = system(strCmd.toStdString().c_str());

    if (!retval) {
        QString cmd = QStringLiteral(u"cd ") + dir + QStringLiteral("\n");
        sendText(cmd);
    }
}

void TerminalSession::setEnvironment(const QStringList &environment)
{
    m_session->setEnvironment(environment);
}

QString TerminalSession::shellProgram() const
{
    return m_session->program();
}

void TerminalSession::setShellProgram(const QString &progname)
{
    m_session->setProgram(progname);
    Q_EMIT shellProgramChanged();
}

void TerminalSession::setInitialWorkingDirectory(const QString &dir)
{
    if (_initialWorkingDirectory != dir) {
        _initialWorkingDirectory = dir;
        m_session->setInitialWorkingDirectory(dir);
        Q_EMIT initialWorkingDirectoryChanged();
    }
}

QString TerminalSession::getInitialWorkingDirectory()
{
    return _initialWorkingDirectory;
}

QStringList TerminalSession::args() const
{
    return m_session->arguments();
}

void TerminalSession::setArgs(const QStringList &args)
{
    m_session->setArguments(args);
    Q_EMIT argsChanged();
}

void TerminalSession::setTextCodec(QTextCodec *codec)
{
    m_session->setCodec(codec);
}

void TerminalSession::setHistorySize(int lines)
{
    if (historySize() != lines) {
        if (lines < 0)
            m_session->setHistoryType(HistoryTypeFile());
        else
            m_session->setHistoryType(HistoryTypeBuffer(lines));
        Q_EMIT historySizeChanged();
    }
}

int TerminalSession::historySize() const
{
    if (m_session->historyType().isUnlimited()) {
        return -1;
    } else {
        return m_session->historyType().maximumLineCount();
    }
}

QString TerminalSession::getHistory() const
{
    QString history;
    QTextStream historyStream(&history);
    PlainTextDecoder historyDecoder;

    historyDecoder.begin(&historyStream);
    m_session->emulation()->writeToStream(&historyDecoder);
    historyDecoder.end();

    return history;
}

void TerminalSession::sendText(QString text)
{
    m_session->sendText(text);
}

void TerminalSession::sendKey(int, int, int) const
{
    // TODO implement or remove this function.
    //    Qt::KeyboardModifier kbm = Qt::KeyboardModifier(mod);

    //    QKeyEvent qkey(QEvent::KeyPress, key, kbm);

    //    while (rep > 0){
    //        m_session->sendKey(&qkey);
    //        --rep;
    //    }
}

void TerminalSession::clearScreen()
{
    m_session->emulation()->clearEntireScreen();
}

void TerminalSession::search(const QString &regexp, int startLine, int startColumn, bool forwards)
{
    HistorySearch *history = new HistorySearch(QPointer<Emulation>(m_session->emulation()), QRegExp(regexp), forwards, startColumn, startLine, this);
    connect(history, &HistorySearch::matchFound, this, &TerminalSession::matchFound);
    connect(history, &HistorySearch::noMatchFound, this, &TerminalSession::noMatchFound);
    history->search();
}

void TerminalSession::setFlowControlEnabled(bool enabled)
{
    m_session->setFlowControlEnabled(enabled);
}

bool TerminalSession::flowControlEnabled()
{
    return m_session->flowControlEnabled();
}

void TerminalSession::setKeyBindings(const QString &kb)
{
    m_session->setKeyBindings(kb);
    Q_EMIT changedKeyBindings(kb);
}

QString TerminalSession::getKeyBindings()
{
    return m_session->keyBindings();
}

QStringList TerminalSession::availableKeyBindings()
{
    return KeyboardTranslatorManager::instance()->allTranslators();
}

QString TerminalSession::keyBindings()
{
    return m_session->keyBindings();
}

QString TerminalSession::getTitle()
{
    return m_session->userTitle();
}

bool TerminalSession::hasActiveProcess() const
{
    return m_session->processId() != m_session->foregroundProcessId();
}

QString TerminalSession::foregroundProcessName()
{
    return m_session->foregroundProcessName();
}

QString TerminalSession::currentDir()
{
    return m_session->currentDir();
}
