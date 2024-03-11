/*
    SPDX-FileCopyrightText: 2007-2008 Robert Knight <robertknight@gmail.com>

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
#include "Filter.h"

// System
#include <iostream>

// Qt
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QSharedData>
#include <QString>
#include <QTextStream>
#include <QUrl>
#include <QtAlgorithms>

// KDE
// #include <KLocale>
// #include <KRun>

// Konsole
#include "TerminalCharacterDecoder.h"
#include "konsole_wcwidth.h"

using namespace Konsole;

FilterChain::~FilterChain() = default;

void FilterChain::addFilter(std::unique_ptr<Filter> &&filter)
{
    push_back(std::move(filter));
}

void FilterChain::removeFilter(Filter *filter)
{
    std::erase_if(*this, [filter](const auto &f) {
        return f.get() == filter;
    });
}

bool FilterChain::containsFilter(Filter *filter)
{
    return std::find_if(this->cbegin(),
                        this->cend(),
                        [filter](const auto &f) {
                            return f.get() == filter;
                        })
        != this->cend();
}

void FilterChain::reset()
{
    for (const auto &filter : *this) {
        filter->reset();
    }
}

void FilterChain::setBuffer(const QString *buffer, const QList<int> *linePositions)
{
    for (const auto &filter : *this) {
        filter->setBuffer(buffer, linePositions);
    }
}

void FilterChain::process()
{
    for (const auto &filter : *this) {
        filter->process();
    }
}

Filter::HotSpot *FilterChain::hotSpotAt(int line, int column) const
{
    for (const auto &filter : *this) {
        Filter::HotSpot *spot = filter->hotSpotAt(line, column);
        if (spot) {
            return spot;
        }
    }

    return nullptr;
}

QList<Filter::HotSpot *> FilterChain::hotSpots() const
{
    QList<Filter::HotSpot *> list;
    for (const auto &filter : *this) {
        for (const auto &hotSpot : filter->hotSpots()) {
            list << hotSpot.get();
        }
    }
    return list;
}
// QList<Filter::HotSpot*> FilterChain::hotSpotsAtLine(int line) const;

TerminalImageFilterChain::TerminalImageFilterChain()
    : _buffer(nullptr)
    , _linePositions(nullptr)
{
}

TerminalImageFilterChain::~TerminalImageFilterChain()
{
    delete _buffer;
    delete _linePositions;
}

void TerminalImageFilterChain::setImage(std::span<const Character> image, int lines, int columns, const QVector<LineProperty> &lineProperties)
{
    if (empty())
        return;

    // reset all filters and hotspots
    reset();

    PlainTextDecoder decoder;
    decoder.setTrailingWhitespace(false);

    // setup new shared buffers for the filters to process on
    QString *newBuffer = new QString();
    QList<int> *newLinePositions = new QList<int>();
    setBuffer(newBuffer, newLinePositions);

    // free the old buffers
    delete _buffer;
    delete _linePositions;

    _buffer = newBuffer;
    _linePositions = newLinePositions;

    QTextStream lineStream(_buffer);
    decoder.begin(&lineStream);

    for (int i = 0; i < lines; i++) {
        _linePositions->append(_buffer->length());
        decoder.decodeLine(image.subspan(i * columns, columns), LINE_DEFAULT);

        // pretend that each line ends with a newline character.
        // this prevents a link that occurs at the end of one line
        // being treated as part of a link that occurs at the start of the next line
        //
        // the downside is that links which are spread over more than one line are not
        // highlighted.
        //
        // TODO - Use the "line wrapped" attribute associated with lines in a
        // terminal image to avoid adding this imaginary character for wrapped
        // lines
        if (!(lineProperties.value(i, LINE_DEFAULT) & LINE_WRAPPED))
            lineStream << QLatin1Char('\n');
    }
    decoder.end();
}

Filter::Filter()
    : _linePositions(nullptr)
    , _buffer(nullptr)
{
}

Filter::~Filter()
{
    _hotspotList.clear();
}
void Filter::reset()
{
    _hotspots.clear();
    _hotspotList.clear();
}

void Filter::setBuffer(const QString *buffer, const QList<int> *linePositions)
{
    _buffer = buffer;
    _linePositions = linePositions;
}

void Filter::getLineColumn(int position, int &startLine, int &startColumn)
{
    Q_ASSERT(_linePositions);
    Q_ASSERT(_buffer);

    for (int i = 0; i < _linePositions->count(); i++) {
        int nextLine = 0;

        if (i == _linePositions->count() - 1)
            nextLine = _buffer->length() + 1;
        else
            nextLine = _linePositions->value(i + 1);

        if (_linePositions->value(i) <= position && position < nextLine) {
            startLine = i;
            startColumn = string_width(buffer()->mid(_linePositions->value(i), position - _linePositions->value(i)));
            return;
        }
    }
}

/*void Filter::addLine(const QString& text)
{
    _linePositions << _buffer.length();
    _buffer.append(text);
}*/

const QString *Filter::buffer()
{
    return _buffer;
}

Filter::HotSpot::~HotSpot() = default;

void Filter::addHotSpot(std::unique_ptr<HotSpot> &&spot)
{
    _hotspotList.push_back(std::move(spot));

    for (int line = spot->startLine(); line <= spot->endLine(); line++) {
        _hotspots.insert({line, std::move(spot)});
    }
}

const std::vector<std::unique_ptr<Filter::HotSpot>> &Filter::hotSpots() const
{
    return _hotspotList;
}

QList<Filter::HotSpot *> Filter::hotSpotsAtLine(int line) const
{
    QList<Filter::HotSpot *> filters;
    for (auto it = _hotspots.find(line); it != _hotspots.cend(); it++) {
        filters.push_back(it->second.get());
    }
    return filters;
}

Filter::HotSpot *Filter::hotSpotAt(int line, int column) const
{
    const auto hotspots = hotSpotsAtLine(line);

    for (const auto spot : hotspots) {
        if (spot->startLine() == line && spot->startColumn() > column)
            continue;
        if (spot->endLine() == line && spot->endColumn() < column)
            continue;

        return spot;
    }

    return nullptr;
}

Filter::HotSpot::HotSpot(int startLine, int startColumn, int endLine, int endColumn)
    : _startLine(startLine)
    , _startColumn(startColumn)
    , _endLine(endLine)
    , _endColumn(endColumn)
    , _type(NotSpecified)
{
}
QList<QAction *> Filter::HotSpot::actions()
{
    return QList<QAction *>();
}
int Filter::HotSpot::startLine() const
{
    return _startLine;
}
int Filter::HotSpot::endLine() const
{
    return _endLine;
}
int Filter::HotSpot::startColumn() const
{
    return _startColumn;
}
int Filter::HotSpot::endColumn() const
{
    return _endColumn;
}
Filter::HotSpot::Type Filter::HotSpot::type() const
{
    return _type;
}
void Filter::HotSpot::setType(Type type)
{
    _type = type;
}

RegExpFilter::RegExpFilter()
{
}

RegExpFilter::HotSpot::HotSpot(int startLine, int startColumn, int endLine, int endColumn)
    : Filter::HotSpot(startLine, startColumn, endLine, endColumn)
{
    setType(Marker);
}

void RegExpFilter::HotSpot::activate(const QString &)
{
}

void RegExpFilter::HotSpot::setCapturedTexts(const QStringList &texts)
{
    _capturedTexts = texts;
}
QStringList RegExpFilter::HotSpot::capturedTexts() const
{
    return _capturedTexts;
}

void RegExpFilter::setRegExp(const QRegExp &regExp)
{
    _searchText = regExp;
}
QRegExp RegExpFilter::regExp() const
{
    return _searchText;
}
/*void RegExpFilter::reset(int)
{
    _buffer = QString();
}*/
void RegExpFilter::process()
{
    int pos = 0;
    const QString *text = buffer();

    Q_ASSERT(text);

    // ignore any regular expressions which match an empty string.
    // otherwise the while loop below will run indefinitely
    static const QString emptyString;
    if (_searchText.exactMatch(emptyString))
        return;

    while (pos >= 0) {
        pos = _searchText.indexIn(*text, pos);

        if (pos >= 0) {
            int startLine = 0;
            int endLine = 0;
            int startColumn = 0;
            int endColumn = 0;

            getLineColumn(pos, startLine, startColumn);
            getLineColumn(pos + _searchText.matchedLength(), endLine, endColumn);

            auto spot = newHotSpot(startLine, startColumn, endLine, endColumn);
            spot->setCapturedTexts(_searchText.capturedTexts());

            addHotSpot(std::move(spot));
            pos += _searchText.matchedLength();

            // if matchedLength == 0, the program will get stuck in an infinite loop
            if (_searchText.matchedLength() == 0)
                pos = -1;
        }
    }
}

std::unique_ptr<RegExpFilter::HotSpot> RegExpFilter::newHotSpot(int startLine, int startColumn, int endLine, int endColumn)
{
    return std::make_unique<RegExpFilter::HotSpot>(startLine, startColumn, endLine, endColumn);
}

std::unique_ptr<RegExpFilter::HotSpot> UrlFilter::newHotSpot(int startLine, int startColumn, int endLine, int endColumn)
{
    auto spot = std::make_unique<UrlFilter::HotSpot>(startLine, startColumn, endLine, endColumn);
    connect(spot->getUrlObject(), &FilterObject::activated, this, &UrlFilter::activated);
    return spot;
}

UrlFilter::HotSpot::HotSpot(int startLine, int startColumn, int endLine, int endColumn)
    : RegExpFilter::HotSpot(startLine, startColumn, endLine, endColumn)
    , _urlObject(new FilterObject(this))
{
    setType(Link);
}

UrlFilter::HotSpot::UrlType UrlFilter::HotSpot::urlType() const
{
    QString url = capturedTexts().constFirst();

    if (FullUrlRegExp.exactMatch(url))
        return StandardUrl;
    else if (EmailAddressRegExp.exactMatch(url))
        return Email;
    else
        return Unknown;
}

void UrlFilter::HotSpot::activate(const QString &actionName)
{
    QString url = capturedTexts().constFirst();

    const UrlType kind = urlType();

    if (actionName == QLatin1String("copy-action")) {
        QApplication::clipboard()->setText(url);
        return;
    }

    if (actionName.isEmpty() || actionName == QLatin1String("open-action") || actionName == QLatin1String("click-action")) {
        if (kind == StandardUrl) {
            // if the URL path does not include the protocol ( eg. "www.kde.org" ) then
            // prepend http:// ( eg. "www.kde.org" --> "http://www.kde.org" )
            if (!url.contains(QLatin1String("://"))) {
                url.prepend(QLatin1String("http://"));
            }
        } else if (kind == Email) {
            url.prepend(QLatin1String("mailto:"));
        }

        _urlObject->emitActivated(QUrl(url, QUrl::StrictMode), actionName != QLatin1String("click-action"));
    }
}

// Note:  Altering these regular expressions can have a major effect on the performance of the filters
// used for finding URLs in the text, especially if they are very general and could match very long
// pieces of text.
// Please be careful when altering them.

// regexp matches:
//  full url:
//  protocolname:// or www. followed by anything other than whitespaces, <, >, ' or ", and ends before whitespaces, <, >, ', ", ], !, comma and dot
const QRegExp UrlFilter::FullUrlRegExp(QLatin1String("(www\\.(?!\\.)|[a-z][a-z0-9+.-]*://)[^\\s<>'\"]+[^!,\\.\\s<>'\"\\]]"));
// email address:
// [word chars, dots or dashes]@[word chars, dots or dashes].[word chars]
const QRegExp UrlFilter::EmailAddressRegExp(QLatin1String("\\b(\\w|\\.|-)+@(\\w|\\.|-)+\\.\\w+\\b"));

// matches full url or email address
const QRegExp UrlFilter::CompleteUrlRegExp(QLatin1Char('(') + FullUrlRegExp.pattern() + QLatin1Char('|') + EmailAddressRegExp.pattern() + QLatin1Char(')'));

UrlFilter::UrlFilter()
{
    setRegExp(CompleteUrlRegExp);
}

UrlFilter::HotSpot::~HotSpot()
{
    delete _urlObject;
}

void FilterObject::emitActivated(const QUrl &url, bool fromContextMenu)
{
    Q_EMIT activated(url, fromContextMenu);
}

void FilterObject::activate()
{
    _filter->activate(sender()->objectName());
}

FilterObject *UrlFilter::HotSpot::getUrlObject() const
{
    return _urlObject;
}

QList<QAction *> UrlFilter::HotSpot::actions()
{
    QList<QAction *> list;

    const UrlType kind = urlType();

    QAction *openAction = new QAction(_urlObject);
    QAction *copyAction = new QAction(_urlObject);
    ;

    Q_ASSERT(kind == StandardUrl || kind == Email);

    if (kind == StandardUrl) {
        openAction->setText(QObject::tr("Open Link"));
        copyAction->setText(QObject::tr("Copy Link Address"));
    } else if (kind == Email) {
        openAction->setText(QObject::tr("Send Email To..."));
        copyAction->setText(QObject::tr("Copy Email Address"));
    }

    // object names are set here so that the hotspot performs the
    // correct action when activated() is called with the triggered
    // action passed as a parameter.
    openAction->setObjectName(QLatin1String("open-action"));
    copyAction->setObjectName(QLatin1String("copy-action"));

    QObject::connect(openAction, &QAction::triggered, _urlObject, &FilterObject::activate);
    QObject::connect(copyAction, &QAction::triggered, _urlObject, &FilterObject::activate);

    list << openAction;
    list << copyAction;

    return list;
}

// #include "Filter.moc"