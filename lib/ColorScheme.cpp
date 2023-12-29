/*
    This source file is part of Konsole, a terminal emulator.

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
#include "ColorScheme.h"
#include "tools.h"

// Qt
#include <QBrush>
#include <QFile>
#include <QFileInfo>
#include <QtDebug>
#include <QSettings>
#include <QDir>
#include <QRegularExpression>
#include <QRandomGenerator>


// KDE
//#include <KColorScheme>
//#include <KConfig>
//#include <KLocale>
//#include <KDebug>
//#include <KConfigGroup>
//#include <KStandardDirs>

using namespace Konsole;

constexpr std::array<ColorEntry, TABLE_COLORS> DEFAULT_TABLE =
 // The following are almost IBM standard color codes, with some slight
 // gamma correction for the dim colors to compensate for bright X screens.
 // It contains the 8 ansiterm/xterm colors in 2 intensities.
{
    ColorEntry(QColor(0x00,0x00,0x00), false), ColorEntry(QColor(0xFF,0xFF,0xFF), true), // Dfore, Dback
    ColorEntry(QColor(0x00,0x00,0x00), false), ColorEntry(QColor(0xB2,0x18,0x18), false), // Black, Red
    ColorEntry(QColor(0x18,0xB2,0x18), false), ColorEntry(QColor(0xB2,0x68,0x18), false), // Green, Yellow
    ColorEntry(QColor(0x18,0x18,0xB2), false), ColorEntry(QColor(0xB2,0x18,0xB2), false), // Blue, Magenta
    ColorEntry(QColor(0x18,0xB2,0xB2), false), ColorEntry(QColor(0xB2,0xB2,0xB2), false), // Cyan, White
    // intensive
    ColorEntry(QColor(0x00,0x00,0x00), false), ColorEntry(QColor(0xFF,0xFF,0xFF), true),
    ColorEntry(QColor(0x68,0x68,0x68), false), ColorEntry(QColor(0xFF,0x54,0x54), false),
    ColorEntry(QColor(0x54,0xFF,0x54), false), ColorEntry(QColor(0xFF,0xFF,0x54), false),
    ColorEntry(QColor(0x54,0x54,0xFF), false), ColorEntry(QColor(0xFF,0x54,0xFF), false),
    ColorEntry(QColor(0x54,0xFF,0xFF), false), ColorEntry(QColor(0xFF,0xFF,0xFF), false)
};

std::array<ColorEntry, TABLE_COLORS> Konsole::defaultColorTable() {
    return DEFAULT_TABLE;
}

constexpr std::array<QStringView, TABLE_COLORS> COLOR_NAMES =
{
    u"Foreground",
    u"Background",
    u"Color0",
    u"Color1",
    u"Color2",
    u"Color3",
    u"Color4",
    u"Color5",
    u"Color6",
    u"Color7",
    u"ForegroundIntense",
    u"BackgroundIntense",
    u"Color0Intense",
    u"Color1Intense",
    u"Color2Intense",
    u"Color3Intense",
    u"Color4Intense",
    u"Color5Intense",
    u"Color6Intense",
    u"Color7Intense"
};

std::array<const char *, TABLE_COLORS> TRANSLATED_COLOR_NAMES =
{
    QT_TR_NOOP("Foreground"),
    QT_TR_NOOP("Background"),
    QT_TR_NOOP("Color 1"),
    QT_TR_NOOP("Color 2"),
    QT_TR_NOOP("Color 3"),
    QT_TR_NOOP("Color 4"),
    QT_TR_NOOP("Color 5"),
    QT_TR_NOOP("Color 6"),
    QT_TR_NOOP("Color 7"),
    QT_TR_NOOP("Color 8"),
    QT_TR_NOOP("Foreground (Intense)"),
    QT_TR_NOOP("Background (Intense)"),
    QT_TR_NOOP("Color 1 (Intense)"),
    QT_TR_NOOP("Color 2 (Intense)"),
    QT_TR_NOOP("Color 3 (Intense)"),
    QT_TR_NOOP("Color 4 (Intense)"),
    QT_TR_NOOP("Color 5 (Intense)"),
    QT_TR_NOOP("Color 6 (Intense)"),
    QT_TR_NOOP("Color 7 (Intense)"),
    QT_TR_NOOP("Color 8 (Intense)")
};

ColorScheme::ColorScheme()
{
    _table = {};
    _randomTable = {};
    _opacity = 1.0;
}

ColorScheme::ColorScheme(const ColorScheme& other)
      : _opacity(other._opacity)
{
    setName(other.name());
    setDescription(other.description());

    _table = other._table;
    _randomTable = other._randomTable;
}
ColorScheme::~ColorScheme()
{
}

void ColorScheme::setDescription(const QString& description) { _description = description; }
QString ColorScheme::description() const { return _description; }

void ColorScheme::setName(const QString& name) { _name = name; }
QString ColorScheme::name() const { return _name; }

void ColorScheme::setColorTableEntry(int index , const ColorEntry& entry)
{
    Q_ASSERT( index >= 0 && index < TABLE_COLORS );

    if ( !_table )
    {
        _table = std::vector<ColorEntry>(DEFAULT_TABLE.begin(), DEFAULT_TABLE.end());
    }

    // we always have a value set at this point, see above
    _table.value()[index] = entry;
}
ColorEntry ColorScheme::colorEntry(int index , uint randomSeed) const
{
    Q_ASSERT( index >= 0 && index < TABLE_COLORS );

    ColorEntry entry = colorTable()[index];

    if ( randomSeed != 0 &&
        _randomTable.has_value() &&
        !_randomTable->data()[index].isNull() )
    {
        const RandomizationRange& range = _randomTable->data()[index];

        int hueDifference = range.hue ? QRandomGenerator::system()->bounded(range.hue) - range.hue/2 : 0;
        int saturationDifference = range.saturation ? (QRandomGenerator::system()->bounded(range.saturation)) - range.saturation/2 : 0;
        int  valueDifference = range.value ? (QRandomGenerator::system()->bounded(range.value)) - range.value/2 : 0;

        QColor& color = entry.color;

        int newHue = qAbs( (color.hue() + hueDifference) % MAX_HUE );
        int newValue = qMin( qAbs(color.value() + valueDifference) , 255 );
        int newSaturation = qMin( qAbs(color.saturation() + saturationDifference) , 255 );

        color.setHsv(newHue,newSaturation,newValue);
    }

    return entry;
}

std::array<ColorEntry, TABLE_COLORS> ColorScheme::getColorTable(uint randomSeed) const
{
    std::array<ColorEntry, TABLE_COLORS> table;
    for ( int i = 0 ; i < TABLE_COLORS ; i++ )
        table[i] = colorEntry(i,randomSeed);

    return table;
}

bool ColorScheme::randomizedBackgroundColor() const
{
    return _randomTable.has_value() ? !_randomTable->data()[1].isNull() : false;
}

void ColorScheme::setRandomizedBackgroundColor(bool randomize)
{
    // the hue of the background colour is allowed to be randomly
    // adjusted as much as possible.
    //
    // the value and saturation are left alone to maintain read-ability
    if ( randomize )
    {
        setRandomizationRange( 1 /* background color index */ , MAX_HUE , 255 , 0 );
    }
    else
    {
        if ( _randomTable )
            setRandomizationRange( 1 /* background color index */ , 0 , 0 , 0 );
    }
}

void ColorScheme::setRandomizationRange( int index , quint16 hue , quint8 saturation ,
                                         quint8 value )
{
    Q_ASSERT( hue <= MAX_HUE );
    Q_ASSERT( index >= 0 && index < TABLE_COLORS );

    if ( !_randomTable )
        _randomTable = std::vector<RandomizationRange>(TABLE_COLORS);

    _randomTable->data()[index].hue = hue;
    _randomTable->data()[index].value = value;
    _randomTable->data()[index].saturation = saturation;
}

const std::span<const ColorEntry> ColorScheme::colorTable() const
{
    if ( _table.has_value() )
        return *_table;
    else
        return std::span(DEFAULT_TABLE);
}

QColor ColorScheme::foregroundColor() const
{
    return colorTable()[0].color;
}

QColor ColorScheme::backgroundColor() const
{
    return colorTable()[1].color;
}

bool ColorScheme::hasDarkBackground() const
{
    // value can range from 0 - 255, with larger values indicating higher brightness.
    // so 127 is in the middle, anything less is deemed 'dark'
    return backgroundColor().value() < 127;
}
void ColorScheme::setOpacity(qreal opacity) { _opacity = opacity; }
qreal ColorScheme::opacity() const { return _opacity; }

void ColorScheme::read(const QString & fileName)
{
    QSettings s(fileName, QSettings::IniFormat);
    s.beginGroup(QLatin1String("General"));

    _description = s.value(QLatin1String("Description"), QObject::tr("Un-named Color Scheme")).toString();
    _opacity = s.value(QLatin1String("Opacity"),qreal(1.0)).toDouble();
    s.endGroup();

    for (int i=0 ; i < TABLE_COLORS ; i++)
    {
        readColorEntry(&s, i);
    }
}

QString ColorScheme::colorNameForIndex(int index)
{
    Q_ASSERT( index >= 0 && index < TABLE_COLORS );

    return COLOR_NAMES.at(index).toString();
}
QString ColorScheme::translatedColorNameForIndex(int index)
{
    Q_ASSERT( index >= 0 && index < TABLE_COLORS );

    return QObject::tr(TRANSLATED_COLOR_NAMES.at(index));
}

void ColorScheme::readColorEntry(QSettings * s , int index)
{
    QString colorName = colorNameForIndex(index);

    s->beginGroup(colorName);

    ColorEntry entry;

    QVariant colorValue = s->value(QLatin1String("Color"));
    QString colorStr;
    int r, g, b;
    bool ok = false;
    // XXX: Undocumented(?) QSettings behavior: values with commas are parsed
    // as QStringList and others QString
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (colorValue.type() == QVariant::StringList)
#else
    if (colorValue.metaType().id() == QMetaType::QStringList)
#endif
    {
        QStringList rgbList = colorValue.toStringList();
        colorStr = rgbList.join(QLatin1Char(','));
        if (rgbList.size() == 3)
        {
            bool parse_ok;

            ok = true;
            r = rgbList[0].toInt(&parse_ok);
            ok = ok && parse_ok && (r >= 0 && r <= 0xff);
            g = rgbList[1].toInt(&parse_ok);
            ok = ok && parse_ok && (g >= 0 && g <= 0xff);
            b = rgbList[2].toInt(&parse_ok);
            ok = ok && parse_ok && (b >= 0 && b <= 0xff);
        }
    }
    else
    {
        colorStr = colorValue.toString();
        QRegularExpression hexColorPattern(QLatin1String("^#[0-9a-f]{6}$"),
                                           QRegularExpression::CaseInsensitiveOption);
        if (hexColorPattern.match(colorStr).hasMatch())
        {
            // Parsing is always ok as already matched by the regexp
            r = QStringView(colorStr).mid(1, 2).toInt(nullptr, 16);
            g = QStringView(colorStr).mid(3, 2).toInt(nullptr, 16);
            b = QStringView(colorStr).mid(5, 2).toInt(nullptr, 16);
            ok = true;
        }
    }
    if (!ok)
    {
        qWarning().nospace() << "Invalid color value " << colorStr
                             << " for " << colorName << ". Fallback to black.";
        r = g = b = 0;
    }
    entry.color = QColor(r, g, b);

    entry.transparent = s->value(QLatin1String("Transparent"),false).toBool();

    // Deprecated key from KDE 4.0 which set 'Bold' to true to force
    // a color to be bold or false to use the current format
    //
    // TODO - Add a new tri-state key which allows for bold, normal or
    // current format
    if (s->contains(QLatin1String("Bold")))
        entry.fontWeight = s->value(QLatin1String("Bold"),false).toBool() ? ColorEntry::Bold :
                                                                 ColorEntry::UseCurrentFormat;

    quint16 hue = s->value(QLatin1String("MaxRandomHue"),0).toInt();
    quint8 value = s->value(QLatin1String("MaxRandomValue"),0).toInt();
    quint8 saturation = s->value(QLatin1String("MaxRandomSaturation"),0).toInt();

    setColorTableEntry( index , entry );

    if ( hue != 0 || value != 0 || saturation != 0 )
       setRandomizationRange( index , hue , saturation , value );

    s->endGroup();
}

//
// Work In Progress - A color scheme for use on KDE setups for users
// with visual disabilities which means that they may have trouble
// reading text with the supplied color schemes.
//
// This color scheme uses only the 'safe' colors defined by the
// KColorScheme class.
//
// A complication this introduces is that each color provided by
// KColorScheme is defined as a 'background' or 'foreground' color.
// Only foreground colors are allowed to be used to render text and
// only background colors are allowed to be used for backgrounds.
//
// The ColorEntry and TerminalDisplay classes do not currently
// support this restriction.
//
// Requirements:
//  - A color scheme which uses only colors from the KColorScheme class
//  - Ability to restrict which colors the TerminalDisplay widget
//    uses as foreground and background color
//  - Make use of KGlobalSettings::allowDefaultBackgroundImages() as
//    a hint to determine whether this accessible color scheme should
//    be used by default.
//
//
// -- Robert Knight <robertknight@gmail.com> 21/07/2007
//
AccessibleColorScheme::AccessibleColorScheme()
    : ColorScheme()
{
#if 0
// It's not finished in konsole and it breaks Qt4 compilation as well
    // basic attributes
    setName("accessible");
    setDescription(QObject::tr("Accessible Color Scheme"));

    // setup colors
    const int ColorRoleCount = 8;

    const KColorScheme colorScheme(QPalette::Active);

    QBrush colors[ColorRoleCount] =
    {
        colorScheme.foreground( colorScheme.NormalText ),
        colorScheme.background( colorScheme.NormalBackground ),

        colorScheme.foreground( colorScheme.InactiveText ),
        colorScheme.foreground( colorScheme.ActiveText ),
        colorScheme.foreground( colorScheme.LinkText ),
        colorScheme.foreground( colorScheme.VisitedText ),
        colorScheme.foreground( colorScheme.NegativeText ),
        colorScheme.foreground( colorScheme.NeutralText )
    };

    for ( int i = 0 ; i < TABLE_COLORS ; i++ )
    {
        ColorEntry entry;
        entry.color = colors[ i % ColorRoleCount ].color();

        setColorTableEntry( i , entry );
    }
#endif
}

ColorSchemeManager::ColorSchemeManager()
    : _haveLoadedAll(false)
{
}

ColorSchemeManager::~ColorSchemeManager() = default;

void ColorSchemeManager::loadAllColorSchemes()
{
    //qDebug() << "loadAllColorSchemes";
    int failed = 0;

    QList<QString> nativeColorSchemes = listColorSchemes();
    QListIterator<QString> nativeIter(nativeColorSchemes);
    while ( nativeIter.hasNext() )
    {
        if ( !loadColorScheme( nativeIter.next() ) )
            failed++;
    }

    /*if ( failed > 0 )
        qDebug() << "failed to load " << failed << " color schemes.";*/

    _haveLoadedAll = true;
}
QList<ColorScheme *> ColorSchemeManager::allColorSchemes()
{
    if ( !_haveLoadedAll )
    {
        loadAllColorSchemes();
    }

    QList<ColorScheme *> schemes;
    for (const auto &[k, scheme] : _colorSchemes) {
        schemes.push_back(scheme.get());
    }
    return schemes;
}

#if 0
void ColorSchemeManager::addColorScheme(ColorScheme* scheme)
{
    _colorSchemes.insert(scheme->name(),scheme);

    // save changes to disk
    QString path = KGlobal::dirs()->saveLocation("data","konsole/") + scheme->name() + ".colorscheme";
    KConfig config(path , KConfig::NoGlobals);

    scheme->write(config);
}
#endif

bool ColorSchemeManager::loadCustomColorScheme(const QString& path)
{
    if (path.endsWith(QLatin1String(".colorscheme")))
        return loadColorScheme(path);

    return false;
}

void ColorSchemeManager::addCustomColorSchemeDir(const QString& custom_dir)
{
    add_custom_color_scheme_dir(custom_dir);
}

bool ColorSchemeManager::loadColorScheme(const QString& filePath)
{
    if ( !filePath.endsWith(QLatin1String(".colorscheme")) || !QFile::exists(filePath) )
        return false;

    QFileInfo info(filePath);

    const QString& schemeName = info.baseName();

    auto scheme = std::make_unique<ColorScheme>();
    scheme->setName(schemeName);
    scheme->read(filePath);

    if (scheme->name().isEmpty())
    {
        //qDebug() << "Color scheme in" << filePath << "does not have a valid name and was not loaded.";
        scheme.reset();
        return false;
    }

    if ( !_colorSchemes.contains(schemeName) )
    {
        _colorSchemes.insert_or_assign(schemeName, std::move(scheme));
    }
    else
    {
        /*qDebug() << "color scheme with name" << schemeName << "has already been" <<
            "found, ignoring.";*/

        scheme.reset();
    }

    return true;
}
QList<QString> ColorSchemeManager::listColorSchemes()
{
    QList<QString> ret;
    for (const QString &scheme_dir : colorSchemesDirs())
    {
        const QString dname(scheme_dir);
        QDir dir(dname);
        QStringList filters;
        filters << QLatin1String("*.colorscheme");
        dir.setNameFilters(filters);
        const QStringList list = dir.entryList(filters);
        for (const QString &i : list)
            ret << dname + QLatin1Char('/') + i;
    }
    return ret;
//    return KGlobal::dirs()->findAllResources("data",
//                                             "konsole/*.colorscheme",
//                                             KStandardDirs::NoDuplicates);
}
const ColorScheme ColorSchemeManager::_defaultColorScheme;
const ColorScheme* ColorSchemeManager::defaultColorScheme() const
{
    return &_defaultColorScheme;
}
bool ColorSchemeManager::deleteColorScheme(const QString& name)
{
    Q_ASSERT( _colorSchemes.contains(name) );

    // lookup the path and delete
    QString path = findColorSchemePath(name);
    if ( QFile::remove(path) )
    {
        _colorSchemes.erase(name);
        return true;
    }
    else
    {
        //qDebug() << "Failed to remove color scheme -" << path;
        return false;
    }
}
QString ColorSchemeManager::findColorSchemePath(const QString& name) const
{
//    QString path = KStandardDirs::locate("data","konsole/"+name+".colorscheme");
    const QStringList dirs = colorSchemesDirs();
    if ( dirs.isEmpty() )
        return QString();

    const QString dir = dirs.first();
    QString path(dir + QLatin1Char('/')+ name + QLatin1String(".colorscheme"));
    if ( !path.isEmpty() )
        return path;

    //path = KStandardDirs::locate("data","konsole/"+name+".schema");
    path = dir + QLatin1Char('/')+ name + QLatin1String(".schema");

    return path;
}
const ColorScheme* ColorSchemeManager::findColorScheme(const QString& name)
{
    if ( name.isEmpty() )
        return defaultColorScheme();

    if ( _colorSchemes.contains(name) )
        return _colorSchemes[name].get();
    else
    {
        // look for this color scheme
        QString path = findColorSchemePath(name);
        if ( !path.isEmpty() && loadColorScheme(path) )
        {
            return findColorScheme(name);
        }

        //qDebug() << "Could not find color scheme - " << name;

        return nullptr;
    }
}
Q_GLOBAL_STATIC(ColorSchemeManager, theColorSchemeManager)
ColorSchemeManager* ColorSchemeManager::instance()
{
    return theColorSchemeManager;
}
