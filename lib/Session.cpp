/*
    This file is part of Konsole

    SPDX-FileCopyrightText: 2006-2007 Robert Knight <robertknight@gmail.com>
    SPDX-FileCopyrightText: 1997, 1998 Lars Doelle <lars.doelle@on-line.de>

    Rewritten for QT4 by e_k <e_k at users.sourceforge.net>, Copyright (C)2008

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
#include "Session.h"

// Standard
#include <csignal>
#include <cstdlib>

// Qt
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QRegularExpression>
#include <QStringList>
#include <QtDebug>

#include "Pty.h"
// #include "kptyprocess.h"
#include "ShellCommand.h"
#include "TerminalDisplay.h"
#include "Vt102Emulation.h"
#include "kptydevice.h"

// QMLTermWidget
#include <QQuickWindow>

using namespace Konsole;

int Session::lastSessionId = 0;

Session::Session(QObject *parent)
    : QObject(parent)
    , _shellProcess(nullptr)
    , _emulation(nullptr)
    , _monitorActivity(false)
    , _monitorSilence(false)
    , _notifiedActivity(false)
    , _autoClose(true)
    , _wantedClose(false)
    , _silenceSeconds(10)
    , _isTitleChanged(false)
    , _addToUtmp(false) // disabled by default because of a bug encountered on certain systems
    // which caused Konsole to hang when closing a tab and then opening a new
    // one.  A 'QProcess destroyed while still running' warning was being
    // printed to the terminal.  Likely a problem in KPty::logout()
    // or KPty::login() which uses a QProcess to start /usr/bin/utempter
    , _flowControl(true)
    , _fullScripting(false)
    , _sessionId(0)
    , _hasDarkBackground(false)
    , _foregroundPid(0)
{
    _sessionId = ++lastSessionId;

    // create teletype for I/O with shell process
    _shellProcess = std::make_unique<Pty>();
    ptySlaveFd = _shellProcess->pty()->slaveFd();

    // create emulation backend
    _emulation = std::make_unique<Vt102Emulation>();

    connect(_emulation.get(), &Emulation::titleChanged, this, &Session::setUserTitle);
    connect(_emulation.get(), &Emulation::stateSet, this, &Session::activityStateSet);
    connect(_emulation.get(), &Emulation::changeTabTextColorRequest, this, &Session::changeTabTextColorRequest);
    connect(_emulation.get(), &Emulation::profileChangeCommandReceived, this, &Session::profileChangeCommandReceived);

    connect(_emulation.get(), &Emulation::imageResizeRequest, this, &Session::onEmulationSizeChange);
    connect(_emulation.get(), &Emulation::imageSizeChanged, this, &Session::onViewSizeChange);
    connect(_emulation.get(), &Vt102Emulation::cursorChanged, this, &Session::cursorChanged);

    // connect teletype to emulation backend
    _shellProcess->setUtf8Mode(_emulation->utf8());

    connect(_shellProcess.get(), &Pty::receivedData, this, &Session::onReceiveBlock);
    connect(_emulation.get(), &Emulation::sendData, _shellProcess.get(), &Pty::sendData);
    connect(_emulation.get(), &Emulation::useUtf8Request, _shellProcess.get(), &Pty::setUtf8Mode);

    connect(_shellProcess.get(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &Session::done);
    // not in kprocess anymore connect( _shellProcess,SIGNAL(done(int)), this, SLOT(done(int)) );

    // setup timer for monitoring session activity
    _monitorTimer = new QTimer(this);
    _monitorTimer->setSingleShot(true);
    connect(_monitorTimer, &QTimer::timeout, this, &Session::monitorTimerDone);
}

WId Session::windowId() const
{
    // On Qt5, requesting window IDs breaks QQuickWidget and the likes,
    // for example, see the following bug reports:
    // https://bugreports.qt.io/browse/QTBUG-40765
    // https://codereview.qt-project.org/#/c/94880/
    return 0;
}

void Session::setDarkBackground(bool darkBackground)
{
    _hasDarkBackground = darkBackground;
}
bool Session::hasDarkBackground() const
{
    return _hasDarkBackground;
}
bool Session::isRunning() const
{
    return _shellProcess->state() == QProcess::Running;
}

void Session::setCodec(QTextCodec *codec) const
{
    emulation()->setCodec(codec);
}

void Session::setProgram(const QString &program)
{
    _program = ShellCommand::expand(program);
}
void Session::setInitialWorkingDirectory(const QString &dir)
{
    _initialWorkingDir = ShellCommand::expand(dir);
}
void Session::setArguments(const QStringList &arguments)
{
    _arguments = ShellCommand::expand(arguments);
}

TerminalDisplay *Session::view() const
{
    return _view;
}

void Session::setView(TerminalDisplay *widget)
{
    Q_ASSERT(!_view);
    _view = widget;

    if (_emulation != nullptr) {
        // connect emulation - view signals and slots
        connect(widget, &TerminalDisplay::keyPressedSignal, _emulation.get(), &Emulation::sendKeyEvent);
        connect(widget, &TerminalDisplay::mouseSignal, _emulation.get(), &Emulation::sendMouseEvent);
        connect(widget, &TerminalDisplay::sendStringToEmu, _emulation.get(), [this](auto string) {
            _emulation->sendString(string);
        });

        // allow emulation to notify view when the foreground process
        // indicates whether or not it is interested in mouse signals
        connect(_emulation.get(), &Emulation::programUsesMouseChanged, widget, &TerminalDisplay::setUsesMouse);

        widget->setUsesMouse(_emulation->programUsesMouse());

        connect(_emulation.get(), &Emulation::programBracketedPasteModeChanged, widget, &TerminalDisplay::setBracketedPasteMode);

        widget->setBracketedPasteMode(_emulation->programBracketedPasteMode());

        widget->setScreenWindow(_emulation->createWindow());
    }

    // connect view signals and slots
    QObject::connect(widget, &TerminalDisplay::changedContentSizeSignal, this, &Session::onViewSizeChange);

    QObject::connect(widget, &QObject::destroyed, this, &Session::viewDestroyed);
    // slot for close
    // QObject::connect(this, SIGNAL(finished()), widget, SLOT(close()));
}

void Session::viewDestroyed(QObject *view)
{
    TerminalDisplay *display = (TerminalDisplay *)view;
    Q_UNUSED(display)

    Q_ASSERT(_view);

    view = nullptr;
}

void Session::removeView(TerminalDisplay *widget)
{
    _view = nullptr;

    disconnect(widget, nullptr, this, nullptr);

    if (_emulation != nullptr) {
        // disconnect
        //  - key presses signals from widget
        //  - mouse activity signals from widget
        //  - string sending signals from widget
        //
        //  ... and any other signals connected in addView()
        disconnect(widget, nullptr, _emulation.get(), nullptr);

        // disconnect state change signals emitted by emulation
        disconnect(_emulation.get(), nullptr, widget, nullptr);
    }

    // close the session automatically when the last view is removed
    close();
}

void Session::run()
{
    // Upon a KPty error, there is no description on what that error was...
    // Check to see if the given program is executable.

    /* ok iam not exactly sure where _program comes from - however it was set to /bin/bash on my system
     * Thats bad for BSD as its /usr/local/bin/bash there - its also bad for arch as its /usr/bin/bash there too!
     * So i added a check to see if /bin/bash exists - if no then we use $SHELL - if that does not exist either, we fall back to /bin/sh
     * As far as i know /bin/sh exists on every unix system.. You could also just put some ifdef __FREEBSD__ here but i think these 2 filechecks are worth
     * their computing time on any system - especially with the problem on arch linux beeing there too.
     */
    QString exec = QString::fromLocal8Bit(QFile::encodeName(_program));
    // if 'exec' is not specified, fall back to default shell.  if that
    // is not set then fall back to /bin/sh

    // here we expect full path. If there is no fullpath let's expect it's
    // a custom shell (eg. python, etc.) available in the PATH.
    if (exec.startsWith(QLatin1Char('/')) || exec.isEmpty()) {
        const QString defaultShell{QLatin1String("/bin/sh")};

        QFile excheck(exec);
        if (exec.isEmpty() || !excheck.exists()) {
            exec = QString::fromLocal8Bit(qgetenv("SHELL"));
        }
        excheck.setFileName(exec);

        if (exec.isEmpty() || !excheck.exists()) {
            qWarning() << "Neither default shell nor $SHELL is set to a correct path. Fallback to" << defaultShell;
            exec = defaultShell;
        }
    }

    QString cwd = QDir::currentPath();
    if (!_initialWorkingDir.isEmpty()) {
        _shellProcess->setWorkingDirectory(_initialWorkingDir);
    } else {
        _shellProcess->setWorkingDirectory(cwd);
    }

    _shellProcess->setFlowControlEnabled(_flowControl);
    _shellProcess->setEraseChar(_emulation->eraseChar());

    // this is not strictly accurate use of the COLORFGBG variable.  This does not
    // tell the terminal exactly which colors are being used, but instead approximates
    // the color scheme as "black on white" or "white on black" depending on whether
    // the background color is deemed dark or not
    QString backgroundColorHint = _hasDarkBackground ? QLatin1String("COLORFGBG=15;0") : QLatin1String("COLORFGBG=0;15");

    /* if we do all the checking if this shell exists then we use it ;)
     * Dont know about the arguments though.. maybe youll need some more checking im not sure
     * However this works on Arch and FreeBSD now.
     */
    int result = _shellProcess->start(exec, _arguments, _environment << backgroundColorHint);

    if (result < 0) {
        qDebug() << "CRASHED! result: " << result;
        return;
    }

    _shellProcess->setWriteable(false); // We are reachable via kwrited.
    Q_EMIT started();
}

void Session::runEmptyPTY()
{
    _shellProcess->setFlowControlEnabled(_flowControl);
    _shellProcess->setEraseChar(_emulation->eraseChar());
    _shellProcess->setWriteable(false);

    // disconnet send data from emulator to internal terminal process
    disconnect(_emulation.get(), &Emulation::sendData, _shellProcess.get(), &Pty::sendData);

    Q_EMIT started();
}

void Session::setUserTitle(int what, const QString &caption)
{
    // set to true if anything is actually changed (eg. old _nameTitle != new _nameTitle )
    bool modified = false;

    // (btw: what=0 changes _userTitle and icon, what=1 only icon, what=2 only _nameTitle
    if ((what == 0) || (what == 2)) {
        _isTitleChanged = true;
        if (_userTitle != caption) {
            _userTitle = caption;
            modified = true;
        }
    }

    if ((what == 0) || (what == 1)) {
        _isTitleChanged = true;
        if (_iconText != caption) {
            _iconText = caption;
            modified = true;
        }
    }

    if (what == 11) {
        QString colorString = caption.section(QLatin1Char(';'), 0, 0);
        // qDebug() << __FILE__ << __LINE__ << ": setting background colour to " << colorString;
        QColor backColor = QColor(colorString);
        if (backColor.isValid()) { // change color via \033]11;Color\007
            if (backColor != _modifiedBackground) {
                _modifiedBackground = backColor;

                // bail out here until the code to connect the terminal display
                // to the changeBackgroundColor() signal has been written
                // and tested - just so we don't forget to do this.
                Q_ASSERT(0);

                Q_EMIT changeBackgroundColorRequest(backColor);
            }
        }
    }

    if (what == 30) {
        _isTitleChanged = true;
        if (_nameTitle != caption) {
            setTitle(Session::NameRole, caption);
            return;
        }
    }

    if (what == 31) {
        QString cwd = caption;
        cwd = cwd.replace(QRegularExpression(QLatin1String("^~")), QDir::homePath());
        Q_EMIT openUrlRequest(cwd);
    }

    // change icon via \033]32;Icon\007
    if (what == 32) {
        _isTitleChanged = true;
        if (_iconName != caption) {
            _iconName = caption;

            modified = true;
        }
    }

    if (what == 50) {
        Q_EMIT profileChangeCommandReceived(caption);
        return;
    }

    if (modified) {
        Q_EMIT titleChanged();
    }
}

QString Session::userTitle() const
{
    return _userTitle;
}

void Session::setTabTitleFormat(TabTitleContext context, const QString &format)
{
    if (context == LocalTabTitle) {
        _localTabTitleFormat = format;
    } else if (context == RemoteTabTitle) {
        _remoteTabTitleFormat = format;
    }
}
QString Session::tabTitleFormat(TabTitleContext context) const
{
    if (context == LocalTabTitle) {
        return _localTabTitleFormat;
    } else if (context == RemoteTabTitle) {
        return _remoteTabTitleFormat;
    }

    return QString();
}

void Session::monitorTimerDone()
{
    // FIXME: The idea here is that the notification popup will appear to tell the user than output from
    // the terminal has stopped and the popup will disappear when the user activates the session.
    //
    // This breaks with the addition of multiple views of a session.  The popup should disappear
    // when any of the views of the session becomes active

    // FIXME: Make message text for this notification and the activity notification more descriptive.
    if (_monitorSilence) {
        Q_EMIT silence();
        Q_EMIT stateChanged(NOTIFYSILENCE);
    } else {
        Q_EMIT stateChanged(NOTIFYNORMAL);
    }

    _notifiedActivity = false;
}

void Session::activityStateSet(int state)
{
    if (state == NOTIFYBELL) {
        Q_EMIT bellRequest(tr("Bell in session '%1'").arg(_nameTitle));
    } else if (state == NOTIFYACTIVITY) {
        if (_monitorSilence) {
            _monitorTimer->start(_silenceSeconds * 1000);
        }

        if (_monitorActivity) {
            // FIXME:  See comments in Session::monitorTimerDone()
            if (!_notifiedActivity) {
                _notifiedActivity = true;
                Q_EMIT activity();
            }
        }
    }

    if (state == NOTIFYACTIVITY && !_monitorActivity) {
        state = NOTIFYNORMAL;
    }
    if (state == NOTIFYSILENCE && !_monitorSilence) {
        state = NOTIFYNORMAL;
    }

    Q_EMIT stateChanged(state);
}

void Session::onViewSizeChange(int /*height*/, int /*width*/)
{
    updateTerminalSize();
}
void Session::onEmulationSizeChange(QSize size)
{
    setSize(size);
}

void Session::updateTerminalSize()
{
    // backend emulation must have a _terminal of at least 1 column x 1 line in size
    _emulation->setImageSize(_view->lines(), _view->columns());
    _shellProcess->setWindowSize(_view->columns(), _view->lines(), _view->width(), _view->height());
}

void Session::refresh()
{
    // attempt to get the shell process to redraw the display
    //
    // this requires the program running in the shell
    // to cooperate by sending an update in response to
    // a window size change
    //
    // the window size is changed twice, first made slightly larger and then
    // resized back to its normal size so that there is actually a change
    // in the window size (some shells do nothing if the
    // new and old sizes are the same)
    //
    // if there is a more 'correct' way to do this, please
    // send an email with method or patches to konsole-devel@kde.org

    const QSize existingSize = _shellProcess->windowSize();
    const QSize existingPixelSize = _shellProcess->pixelSize();
    _shellProcess->setWindowSize(existingSize.height(), existingSize.width() + 1, existingPixelSize.height(), existingPixelSize.width());
    _shellProcess->setWindowSize(existingSize.height(), existingSize.width(), existingPixelSize.height(), existingPixelSize.width());
}

bool Session::sendSignal(int signal)
{
    int result = ::kill(static_cast<pid_t>(_shellProcess->processId()), signal);

    if (result == 0) {
        _shellProcess->waitForFinished();
        return true;
    } else
        return false;
}

void Session::close()
{
    _autoClose = true;
    _wantedClose = true;
    if (_shellProcess->state() != QProcess::Running || !sendSignal(SIGHUP)) {
        // Forced close.
        QTimer::singleShot(1, this, &Session::finished);
    }
}

void Session::sendText(const QString &text) const
{
    _emulation->sendText(text);
}

void Session::sendKeyEvent(QKeyEvent *e) const
{
    _emulation->sendKeyEvent(e, false);
}

Session::~Session() = default;

void Session::setProfileKey(const QString &key)
{
    _profileKey = key;
    Q_EMIT profileChanged(key);
}

QString Session::profileKey() const
{
    return _profileKey;
}

void Session::done(int exitStatus)
{
    if (!_autoClose) {
        _userTitle = QString::fromLatin1("This session is done. Finished");
        Q_EMIT titleChanged();
        return;
    }

    // message is not being used. But in the original kpty.cpp file
    // (https://cgit.kde.org/kpty.git/) it's part of a notification.
    // So, we make it translatable, hoping that in the future it will
    // be used in some kind of notification.
    QString message;
    if (!_wantedClose || exitStatus != 0) {
        if (_shellProcess->exitStatus() == QProcess::NormalExit) {
            message = tr("Session '%1' exited with status %2.").arg(_nameTitle).arg(exitStatus);
        } else {
            message = tr("Session '%1' crashed.").arg(_nameTitle);
        }
    }

    if (!_wantedClose && _shellProcess->exitStatus() != QProcess::NormalExit)
        message = tr("Session '%1' exited unexpectedly.").arg(_nameTitle);
    else
        Q_EMIT finished();
}

Emulation *Session::emulation() const
{
    return _emulation.get();
}

QString Session::keyBindings() const
{
    return _emulation->keyBindings();
}

QStringList Session::environment() const
{
    return _environment;
}

void Session::setEnvironment(const QStringList &environment)
{
    _environment = environment;
}

int Session::sessionId() const
{
    return _sessionId;
}

void Session::setKeyBindings(const QString &id)
{
    _emulation->setKeyBindings(id);
}

void Session::setTitle(TitleRole role, const QString &newTitle)
{
    if (title(role) != newTitle) {
        if (role == NameRole) {
            _nameTitle = newTitle;
        } else if (role == DisplayedTitleRole) {
            _displayTitle = newTitle;
        }

        Q_EMIT titleChanged();
    }
}

QString Session::title(TitleRole role) const
{
    if (role == NameRole) {
        return _nameTitle;
    } else if (role == DisplayedTitleRole) {
        return _displayTitle;
    } else {
        return QString();
    }
}

void Session::setIconName(const QString &iconName)
{
    if (iconName != _iconName) {
        _iconName = iconName;
        Q_EMIT titleChanged();
    }
}

void Session::setIconText(const QString &iconText)
{
    _iconText = iconText;
    // kDebug(1211)<<"Session setIconText " <<  _iconText;
}

QString Session::iconName() const
{
    return _iconName;
}

QString Session::iconText() const
{
    return _iconText;
}

bool Session::isTitleChanged() const
{
    return _isTitleChanged;
}

void Session::setHistoryType(const HistoryType &hType)
{
    _emulation->setHistory(hType);
}

const HistoryType &Session::historyType() const
{
    return _emulation->history();
}

void Session::clearHistory()
{
    _emulation->clearHistory();
}

QStringList Session::arguments() const
{
    return _arguments;
}

QString Session::program() const
{
    return _program;
}

// unused currently
bool Session::isMonitorActivity() const
{
    return _monitorActivity;
}
// unused currently
bool Session::isMonitorSilence() const
{
    return _monitorSilence;
}

void Session::setMonitorActivity(bool _monitor)
{
    _monitorActivity = _monitor;
    _notifiedActivity = false;

    activityStateSet(NOTIFYNORMAL);
}

void Session::setMonitorSilence(bool _monitor)
{
    if (_monitorSilence == _monitor) {
        return;
    }

    _monitorSilence = _monitor;
    if (_monitorSilence) {
        _monitorTimer->start(_silenceSeconds * 1000);
    } else {
        _monitorTimer->stop();
    }

    activityStateSet(NOTIFYNORMAL);
}

void Session::setMonitorSilenceSeconds(int seconds)
{
    _silenceSeconds = seconds;
    if (_monitorSilence) {
        _monitorTimer->start(_silenceSeconds * 1000);
    }
}

void Session::setAddToUtmp(bool set)
{
    _addToUtmp = set;
}

void Session::setFlowControlEnabled(bool enabled)
{
    if (_flowControl == enabled) {
        return;
    }

    _flowControl = enabled;

    if (_shellProcess) {
        _shellProcess->setFlowControlEnabled(_flowControl);
    }

    Q_EMIT flowControlEnabledChanged(enabled);
}
bool Session::flowControlEnabled() const
{
    return _flowControl;
}

void Session::onReceiveBlock(const char *buf, int len)
{
    _emulation->receiveData(buf, len);
    Q_EMIT receivedData(QString::fromLatin1(buf, len));
}

QSize Session::size()
{
    return _emulation->imageSize();
}

void Session::setSize(const QSize &size)
{
    if ((size.width() <= 1) || (size.height() <= 1)) {
        return;
    }

    Q_EMIT resizeRequest(size);
}
int Session::foregroundProcessId() const
{
    return _shellProcess->foregroundProcessGroup();
}

QString Session::foregroundProcessName()
{
    QString name;

    if (updateForegroundProcessInfo()) {
        bool ok = false;
        name = _foregroundProcessInfo->name(&ok);
        if (!ok)
            name.clear();
    }

    return name;
}

QString Session::currentDir()
{
    QString path;
    if (updateForegroundProcessInfo()) {
        bool ok = false;
        path = _foregroundProcessInfo->currentDir(&ok);
        if (!ok)
            path.clear();
    }
    return path;
}

bool Session::updateForegroundProcessInfo()
{
    Q_ASSERT(_shellProcess);

    const int foregroundPid = _shellProcess->foregroundProcessGroup();
    if (foregroundPid != _foregroundPid) {
        _foregroundProcessInfo.reset();
        _foregroundProcessInfo = ProcessInfo::newInstance(foregroundPid);
        _foregroundPid = foregroundPid;
    }

    if (_foregroundProcessInfo) {
        _foregroundProcessInfo->update();
        return _foregroundProcessInfo->isValid();
    } else {
        return false;
    }
}

int Session::processId() const
{
    return static_cast<int>(_shellProcess->processId());
}
int Session::getPtySlaveFd() const
{
    return ptySlaveFd;
}

SessionGroup::SessionGroup()
    : _masterMode(0)
{
}
SessionGroup::~SessionGroup()
{
    // disconnect all
    connectAll(false);
}
int SessionGroup::masterMode() const
{
    return _masterMode;
}
QList<Session *> SessionGroup::sessions() const
{
    return _sessions.keys();
}
bool SessionGroup::masterStatus(Session *session) const
{
    return _sessions[session];
}

void SessionGroup::addSession(Session *session)
{
    _sessions.insert(session, false);

    const auto masterSessions = masters();
    for (const auto master : masterSessions) {
        connectPair(master, session);
    }
}

void SessionGroup::removeSession(Session *session)
{
    setMasterStatus(session, false);

    const auto masterSessions = masters();
    for (const auto master : masterSessions) {
        disconnectPair(master, session);
    }

    _sessions.remove(session);
}

void SessionGroup::setMasterMode(int mode)
{
    _masterMode = mode;

    connectAll(false);
    connectAll(true);
}

QList<Session *> SessionGroup::masters() const
{
    return _sessions.keys(true);
}

void SessionGroup::connectAll(bool connect)
{
    const auto masterSessions = masters();
    for (const auto master : masterSessions) {
        const auto other = _sessions.keys();

        for (const auto other : other) {
            if (other != master) {
                if (connect) {
                    connectPair(master, other);
                } else {
                    disconnectPair(master, other);
                }
            }
        }
    }
}

void SessionGroup::setMasterStatus(Session *session, bool master)
{
    bool wasMaster = _sessions[session];
    _sessions[session] = master;

    if (wasMaster == master) {
        return;
    }

    const auto otherSessions = _sessions.keys();
    for (const auto other : otherSessions) {
        if (other != session) {
            if (master) {
                connectPair(session, other);
            } else {
                disconnectPair(session, other);
            }
        }
    }
}

void SessionGroup::connectPair(Session *master, Session *other) const
{
    //    qDebug() << k_funcinfo;

    if (_masterMode & CopyInputToAll) {
        qDebug() << "Connection session " << master->nameTitle() << "to" << other->nameTitle();

        connect(master->emulation(), &Emulation::sendData, other->emulation(), &Emulation::sendString);
    }
}
void SessionGroup::disconnectPair(Session *master, Session *other) const
{
    //    qDebug() << k_funcinfo;

    if (_masterMode & CopyInputToAll) {
        qDebug() << "Disconnecting session " << master->nameTitle() << "from" << other->nameTitle();

        disconnect(master->emulation(), &Emulation::sendData, other->emulation(), &Emulation::sendString);
    }
}

// #include "moc_Session.cpp"
