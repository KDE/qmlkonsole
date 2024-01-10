/*
    This file is part of Konsole, a terminal emulator for KDE.

    SPDX-FileCopyrightText: 2006-2008 Robert Knight <robertknight@gmail.com>
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

// Own
#include "TerminalDisplay.h"

// C++
#include <cmath>

// Qt
#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QEvent>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStyle>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QtDebug>

// Konsole
#include "Filter.h"
#include "ScreenWindow.h"
#include "TerminalCharacterDecoder.h"
#include "konsole_wcwidth.h"

// std
#include <ranges>

inline void initResource()
{
    Q_INIT_RESOURCE(terminal);
}

using namespace Konsole;

constexpr auto REPCHAR =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefgjijklmnopqrstuvwxyz"
    "0123456789./+@";

// scroll increment used when dragging selection at top/bottom of window.

// static
bool TerminalDisplay::_antialiasText = true;

// we use this to force QPainter to display text in LTR mode
// more information can be found in: http://unicode.org/reports/tr9/
const QChar LTR_OVERRIDE_CHAR(0x202D);

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                Colors                                     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/* Note that we use ANSI color order (bgr), while IBMPC color order is (rgb)

   Code        0       1       2       3       4       5       6       7
   ----------- ------- ------- ------- ------- ------- ------- ------- -------
   ANSI  (bgr) Black   Red     Green   Yellow  Blue    Magenta Cyan    White
   IBMPC (rgb) Black   Blue    Green   Cyan    Red     Magenta Yellow  White
*/

ScreenWindow *TerminalDisplay::screenWindow() const
{
    return _screenWindow;
}
void TerminalDisplay::setScreenWindow(ScreenWindow *window)
{
    // disconnect existing screen window if any
    if (_screenWindow) {
        disconnect(_screenWindow, nullptr, this, nullptr);
    }

    _screenWindow = window;

    if (window) {
        connect(_screenWindow, &ScreenWindow::outputChanged, this, &TerminalDisplay::updateLineProperties);
        connect(_screenWindow, &ScreenWindow::outputChanged, this, &TerminalDisplay::updateImage);
        connect(_screenWindow, &ScreenWindow::scrollToEnd, this, &TerminalDisplay::scrollToEnd);
        window->setWindowLines(_lines);
    }
}

std::span<const ColorEntry> TerminalDisplay::colorTable() const
{
    return _colorTable;
}

void TerminalDisplay::setBackgroundColor(const QColor &color)
{
    _colorTable[DEFAULT_BACK_COLOR].color = color;
    QPalette p = palette();
    p.setColor(backgroundRole(), color);
    setPalette(p);

    // Avoid propagating the palette change to the scroll bar
    _scrollBar->setPalette(QApplication::palette());

    update();
}

void TerminalDisplay::setForegroundColor(const QColor &color)
{
    _colorTable[DEFAULT_FORE_COLOR].color = color;

    update();
}

void TerminalDisplay::setColorTable(std::array<ColorEntry, TABLE_COLORS> &&table)
{
    _colorTable = std::move(table);
    setBackgroundColor(_colorTable[DEFAULT_BACK_COLOR].color);
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                   Font                                    */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*
   The VT100 has 32 special graphical characters. The usual vt100 extended
   xterm fonts have these at 0x00..0x1f.

   QT's iso mapping leaves 0x00..0x7f without any changes. But the graphicals
   come in here as proper unicode characters.

   We treat non-iso10646 fonts as VT100 extended and do the requiered mapping
   from unicode to 0x00..0x1f. The remaining translation is then left to the
   QCodec.
*/

constexpr bool TerminalDisplay::isLineChar(QChar c) const
{
    return _drawLineChars && ((c.unicode() & 0xFF80) == 0x2500);
}

constexpr bool TerminalDisplay::isLineCharString(QStringView string) const
{
    return (string.size() > 0) && (isLineChar(string[0]));
}

void TerminalDisplay::fontChange(const QFont &)
{
    QFontMetricsF fm(font());
    _fontHeight = fm.height() + _lineSpacing;

    // waba TerminalDisplay 1.123:
    // "Base character width on widest ASCII character. This prevents too wide
    //  characters in the presence of double wide (e.g. Japanese) characters."
    // Get the width from representative normal width characters
    _fontWidth = fm.horizontalAdvance(QLatin1String(REPCHAR)) / (qreal)qstrlen(REPCHAR);

    _fixedFont = true;

    int fw = fm.horizontalAdvance(QLatin1Char(REPCHAR[0]));
    for (unsigned int i = 1; i < qstrlen(REPCHAR); i++) {
        if (fw != fm.horizontalAdvance(QLatin1Char(REPCHAR[i]))) {
            _fixedFont = false;
            break;
        }
    }

    if (_fontWidth < 1)
        _fontWidth = 1;

    _fontAscent = fm.ascent();

    Q_EMIT changedFontMetricSignal(_fontHeight, _fontWidth);
    propagateSize();
    update();
}

// void TerminalDisplay::calDrawTextAdditionHeight(QPainter& painter)
//{
//     QRect test_rect, feedback_rect;
//	test_rect.setRect(1, 1, qRound(_fontWidth) * 4, _fontHeight);
//     painter.drawText(test_rect, Qt::AlignBottom, LTR_OVERRIDE_CHAR + QLatin1String("Mq"), &feedback_rect);

//	//qDebug() << "test_rect:" << test_rect << "feeback_rect:" << feedback_rect;

//	_drawTextAdditionHeight = (feedback_rect.height() - _fontHeight) / 2;
//	if(_drawTextAdditionHeight < 0) {
//	  _drawTextAdditionHeight = 0;
//	}

//  _drawTextTestFlag = false;
//}

void TerminalDisplay::setVTFont(const QFont &f)
{
    QFont font = f;

    if (!QFontInfo(font).fixedPitch()) {
        qDebug() << "Using a variable-width font in the terminal.  This may cause performance degradation and display/alignment errors.";
    }

    // hint that text should be drawn without anti-aliasing.
    // depending on the user's font configuration, this may not be respected
    if (!_antialiasText)
        font.setStyleStrategy(QFont::NoAntialias);

    // experimental optimization.  Konsole assumes that the terminal is using a
    // mono-spaced font, in which case kerning information should have an effect.
    // Disabling kerning saves some computation when rendering text.
    font.setKerning(false);

    m_font = font;
    fontChange(font);
    Q_EMIT vtFontChanged();
}

void TerminalDisplay::setBoldIntense(bool value)
{
    if (_boldIntense != value) {
        _boldIntense = value;
        Q_EMIT boldIntenseChanged();
    }
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                         Constructor / Destructor                          */
/*                                                                           */
/* ------------------------------------------------------------------------- */
#include <QDir>

TerminalDisplay::TerminalDisplay(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , _screenWindow(nullptr)
    , _allowBell(true)
    , _gridLayout(nullptr)
    , _fontHeight(1)
    , _fontWidth(1)
    , _fontAscent(1)
    , _boldIntense(true)
    , _lines(1)
    , _columns(1)
    , _usedLines(1)
    , _usedColumns(1)
    , _contentHeight(1)
    , _contentWidth(1)
    , _randomSeed(0)
    , _resizing(false)
    , _terminalSizeHint(false)
    , _terminalSizeStartup(true)
    , _bidiEnabled(false)
    , _mouseMarks(false)
    , _disabledBracketedPasteMode(false)
    , _actSel(0)
    , _wordSelectionMode(false)
    , _lineSelectionMode(false)
    , _preserveLineBreaks(false)
    , _columnSelectionMode(false)
    , _scrollbarLocation(QTermWidget::NoScrollBar)
    , _wordCharacters(QLatin1String(":@-./_~"))
    , _bellMode(SystemBeepBell)
    , _blinking(false)
    , _hasBlinker(false)
    , _cursorBlinking(false)
    , _hasBlinkingCursor(false)
    , _allowBlinkingText(true)
    , _ctrlDrag(false)
    , _tripleClickMode(SelectWholeLine)
    , _isFixedSize(false)
    , _possibleTripleClick(false)
    , _resizeWidget(nullptr)
    , _resizeTimer(nullptr)
    , _flowControlWarningEnabled(false)
    , _outputSuspendedLabel(nullptr)
    , _lineSpacing(0)
    , _colorsInverted(false)
    , _opacity(static_cast<qreal>(1))
    , _filterChain(std::make_unique<TerminalImageFilterChain>())
    , _cursorShape(Emulation::KeyboardCursorShape::BlockCursor)
    , mMotionAfterPasting(NoMoveScreenWindow)
    , _leftBaseMargin(1)
    , _topBaseMargin(1)
    , m_font(QStringLiteral("Monospace"), 12)
    , m_color_role(QPalette::Window)
    , m_full_cursor_height(false)
    , _drawLineChars(true)
{
    initResource();

    // terminal applications are not designed with Right-To-Left in mind,
    // so the layout is forced to Left-To-Right
    // setLayoutDirection(Qt::LeftToRight);

    // The offsets are not yet calculated.
    // Do not calculate these too often to be more smoothly when resizing
    // konsole in opaque mode.
    _topMargin = _topBaseMargin;
    _leftMargin = _leftBaseMargin;

    m_palette = qApp->palette();

    setVTFont(m_font);

    // create scroll bar for scrolling output up and down
    // set the scroll bar's slider to occupy the whole area of the scroll bar initially

    _scrollBar = new QScrollBar();
    setScroll(0, 0);

    _scrollBar->setCursor(Qt::ArrowCursor);
    connect(_scrollBar, &QAbstractSlider::valueChanged, this, &TerminalDisplay::scrollBarPositionChanged);
    // qtermwidget: we have to hide it here due the _scrollbarLocation==NoScrollBar
    // check in TerminalDisplay::setScrollBarPosition(ScrollBarPosition position)
    _scrollBar->hide();

    // setup timers for blinking cursor and text
    _blinkTimer = new QTimer(this);
    connect(_blinkTimer, SIGNAL(timeout()), this, SLOT(blinkEvent()));
    _blinkCursorTimer = new QTimer(this);
    connect(_blinkCursorTimer, &QTimer::timeout, this, &TerminalDisplay::blinkCursorEvent);

    //  KCursor::setAutoHideCursor( this, true );

    setUsesMouse(true);
    setBracketedPasteMode(false);
    setColorTable(defaultColorTable());
    // setMouseTracking(true);

    setAcceptedMouseButtons(Qt::LeftButton);

    setFlags(ItemHasContents | ItemAcceptsInputMethod);

    // Setup scrollbar. Be sure it is not darw on screen.
    _scrollBar->setAttribute(Qt::WA_DontShowOnScreen);
    _scrollBar->setVisible(false);
    connect(_scrollBar, &QAbstractSlider::valueChanged, this, &TerminalDisplay::scrollbarParamsChanged);

    // TODO Forcing rendering to Framebuffer. We need to determine if this is ok
    // always or if we need to make this customizable.
    setRenderTarget(QQuickPaintedItem::FramebufferObject);

    //  setFocusPolicy( Qt::WheelFocus );

    // enable input method support
    //  setAttribute(Qt::WA_InputMethodEnabled, true);

    // this is an important optimization, it tells Qt
    // that TerminalDisplay will handle repainting its entire area.
    //  setAttribute(Qt::WA_OpaquePaintEvent);

    //  _gridLayout = new QGridLayout(this);
    //  _gridLayout->setContentsMargins(0, 0, 0, 0);

    //  new AutoScrollHandler(this);
}

TerminalDisplay::~TerminalDisplay()
{
    disconnect(_blinkTimer);
    disconnect(_blinkCursorTimer);
    qApp->removeEventFilter(this);

    delete _gridLayout;
    delete _outputSuspendedLabel;
    delete _scrollBar;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                             Display Operations                            */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/**
 A table for emulating the simple (single width) unicode drawing chars.
 It represents the 250x - 257x glyphs. If it's zero, we can't use it.
 if it's not, it's encoded as follows: imagine a 5x5 grid where the points are numbered
 0 to 24 left to top, top to bottom. Each point is represented by the corresponding bit.

 Then, the pixels basically have the following interpretation:
 _|||_
 -...-
 -...-
 -...-
 _|||_

where _ = none
      | = vertical line.
      - = horizontal line.
 */

enum LineEncode {
    TopL = (1 << 1),
    TopC = (1 << 2),
    TopR = (1 << 3),

    LeftT = (1 << 5),
    Int11 = (1 << 6),
    Int12 = (1 << 7),
    Int13 = (1 << 8),
    RightT = (1 << 9),

    LeftC = (1 << 10),
    Int21 = (1 << 11),
    Int22 = (1 << 12),
    Int23 = (1 << 13),
    RightC = (1 << 14),

    LeftB = (1 << 15),
    Int31 = (1 << 16),
    Int32 = (1 << 17),
    Int33 = (1 << 18),
    RightB = (1 << 19),

    BotL = (1 << 21),
    BotC = (1 << 22),
    BotR = (1 << 23)
};

#include "LineFont.h"

static void drawLineChar(QPainter &paint, int x, int y, int w, int h, uint8_t code)
{
    // Calculate cell midpoints, end points.
    int cx = x + w / 2;
    int cy = y + h / 2;
    int ex = x + w - 1;
    int ey = y + h - 1;

    quint32 toDraw = LineChars[code];

    // Top _lines:
    if (toDraw & TopL)
        paint.drawLine(cx - 1, y, cx - 1, cy - 2);
    if (toDraw & TopC)
        paint.drawLine(cx, y, cx, cy - 2);
    if (toDraw & TopR)
        paint.drawLine(cx + 1, y, cx + 1, cy - 2);

    // Bot _lines:
    if (toDraw & BotL)
        paint.drawLine(cx - 1, cy + 2, cx - 1, ey);
    if (toDraw & BotC)
        paint.drawLine(cx, cy + 2, cx, ey);
    if (toDraw & BotR)
        paint.drawLine(cx + 1, cy + 2, cx + 1, ey);

    // Left _lines:
    if (toDraw & LeftT)
        paint.drawLine(x, cy - 1, cx - 2, cy - 1);
    if (toDraw & LeftC)
        paint.drawLine(x, cy, cx - 2, cy);
    if (toDraw & LeftB)
        paint.drawLine(x, cy + 1, cx - 2, cy + 1);

    // Right _lines:
    if (toDraw & RightT)
        paint.drawLine(cx + 2, cy - 1, ex, cy - 1);
    if (toDraw & RightC)
        paint.drawLine(cx + 2, cy, ex, cy);
    if (toDraw & RightB)
        paint.drawLine(cx + 2, cy + 1, ex, cy + 1);

    // Intersection points.
    if (toDraw & Int11)
        paint.drawPoint(cx - 1, cy - 1);
    if (toDraw & Int12)
        paint.drawPoint(cx, cy - 1);
    if (toDraw & Int13)
        paint.drawPoint(cx + 1, cy - 1);

    if (toDraw & Int21)
        paint.drawPoint(cx - 1, cy);
    if (toDraw & Int22)
        paint.drawPoint(cx, cy);
    if (toDraw & Int23)
        paint.drawPoint(cx + 1, cy);

    if (toDraw & Int31)
        paint.drawPoint(cx - 1, cy + 1);
    if (toDraw & Int32)
        paint.drawPoint(cx, cy + 1);
    if (toDraw & Int33)
        paint.drawPoint(cx + 1, cy + 1);
}

static void drawOtherChar(QPainter &paint, int x, int y, int w, int h, uchar code)
{
    // Calculate cell midpoints, end points.
    const int cx = x + w / 2;
    const int cy = y + h / 2;
    const int ex = x + w - 1;
    const int ey = y + h - 1;

    // Double dashes
    if (0x4C <= code && code <= 0x4F) {
        const int xHalfGap = qMax(w / 15, 1);
        const int yHalfGap = qMax(h / 15, 1);
        switch (code) {
        case 0x4D: // BOX DRAWINGS HEAVY DOUBLE DASH HORIZONTAL
            paint.drawLine(x, cy - 1, cx - xHalfGap - 1, cy - 1);
            paint.drawLine(x, cy + 1, cx - xHalfGap - 1, cy + 1);
            paint.drawLine(cx + xHalfGap, cy - 1, ex, cy - 1);
            paint.drawLine(cx + xHalfGap, cy + 1, ex, cy + 1);
            /* Falls through. */
        case 0x4C: // BOX DRAWINGS LIGHT DOUBLE DASH HORIZONTAL
            paint.drawLine(x, cy, cx - xHalfGap - 1, cy);
            paint.drawLine(cx + xHalfGap, cy, ex, cy);
            break;
        case 0x4F: // BOX DRAWINGS HEAVY DOUBLE DASH VERTICAL
            paint.drawLine(cx - 1, y, cx - 1, cy - yHalfGap - 1);
            paint.drawLine(cx + 1, y, cx + 1, cy - yHalfGap - 1);
            paint.drawLine(cx - 1, cy + yHalfGap, cx - 1, ey);
            paint.drawLine(cx + 1, cy + yHalfGap, cx + 1, ey);
            /* Falls through. */
        case 0x4E: // BOX DRAWINGS LIGHT DOUBLE DASH VERTICAL
            paint.drawLine(cx, y, cx, cy - yHalfGap - 1);
            paint.drawLine(cx, cy + yHalfGap, cx, ey);
            break;
        }
    }

    // Rounded corner characters
    else if (0x6D <= code && code <= 0x70) {
        const int r = w * 3 / 8;
        const int d = 2 * r;
        switch (code) {
        case 0x6D: // BOX DRAWINGS LIGHT ARC DOWN AND RIGHT
            paint.drawLine(cx, cy + r, cx, ey);
            paint.drawLine(cx + r, cy, ex, cy);
            paint.drawArc(cx, cy, d, d, 90 * 16, 90 * 16);
            break;
        case 0x6E: // BOX DRAWINGS LIGHT ARC DOWN AND LEFT
            paint.drawLine(cx, cy + r, cx, ey);
            paint.drawLine(x, cy, cx - r, cy);
            paint.drawArc(cx - d, cy, d, d, 0 * 16, 90 * 16);
            break;
        case 0x6F: // BOX DRAWINGS LIGHT ARC UP AND LEFT
            paint.drawLine(cx, y, cx, cy - r);
            paint.drawLine(x, cy, cx - r, cy);
            paint.drawArc(cx - d, cy - d, d, d, 270 * 16, 90 * 16);
            break;
        case 0x70: // BOX DRAWINGS LIGHT ARC UP AND RIGHT
            paint.drawLine(cx, y, cx, cy - r);
            paint.drawLine(cx + r, cy, ex, cy);
            paint.drawArc(cx, cy - d, d, d, 180 * 16, 90 * 16);
            break;
        }
    }

    // Diagonals
    else if (0x71 <= code && code <= 0x73) {
        switch (code) {
        case 0x71: // BOX DRAWINGS LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT
            paint.drawLine(ex, y, x, ey);
            break;
        case 0x72: // BOX DRAWINGS LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT
            paint.drawLine(x, y, ex, ey);
            break;
        case 0x73: // BOX DRAWINGS LIGHT DIAGONAL CROSS
            paint.drawLine(ex, y, x, ey);
            paint.drawLine(x, y, ex, ey);
            break;
        }
    }
}

void TerminalDisplay::drawLineCharString(QPainter &painter, int x, int y, QStringView str, const Character *attributes) const
{
    const QPen &currentPen = painter.pen();

    if ((attributes->rendition & RE_BOLD) && _boldIntense) {
        QPen boldPen(currentPen);
        boldPen.setWidth(3);
        painter.setPen(boldPen);
    }

    for (qsizetype i = 0; i < str.size(); i++) {
        uint8_t code = static_cast<uint8_t>(str[i].unicode() & 0xffU);
        if (LineChars[code])
            drawLineChar(painter, qRound(x + (_fontWidth * i)), y, qRound(_fontWidth), qRound(_fontHeight), code);
        else
            drawOtherChar(painter, qRound(x + (_fontWidth * i)), y, qRound(_fontWidth), qRound(_fontHeight), code);
    }

    painter.setPen(currentPen);
}

void TerminalDisplay::setKeyboardCursorShape(Emulation::KeyboardCursorShape shape)
{
    if (_cursorShape == shape) {
        return;
    }

    _cursorShape = shape;
    updateCursor();
}

Emulation::KeyboardCursorShape TerminalDisplay::keyboardCursorShape() const
{
    return _cursorShape;
}

void TerminalDisplay::setKeyboardCursorColor(bool useForegroundColor, const QColor &color)
{
    if (useForegroundColor)
        _cursorColor = QColor(); // an invalid color means that
                                 // the foreground color of the
                                 // current character should
                                 // be used

    else
        _cursorColor = color;
}
QColor TerminalDisplay::keyboardCursorColor() const
{
    return _cursorColor;
}

void TerminalDisplay::setOpacity(qreal opacity)
{
    _opacity = qBound(static_cast<qreal>(0), opacity, static_cast<qreal>(1));
    update();
}

void TerminalDisplay::drawBackground(QPainter &painter, const QRect &rect, const QColor &backgroundColor, bool useOpacitySetting)
{
    // The whole widget rectangle is filled by the background color from
    // the color scheme set in setColorTable(), while the scrollbar is
    // left to the widget style for a consistent look.
    if (useOpacitySetting) {
        QColor color(backgroundColor);
        color.setAlphaF(_opacity);

        painter.save();
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect, color);
        painter.restore();
    } else
        painter.fillRect(rect, backgroundColor);
}

void TerminalDisplay::drawCursor(QPainter &painter,
                                 const QRect &rect,
                                 const QColor &foregroundColor,
                                 const QColor & /*backgroundColor*/,
                                 bool &invertCharacterColor)
{
    QRect cursorRect = rect;

    cursorRect.setHeight(qRound(_fontHeight) - ((m_full_cursor_height) ? 0 : _lineSpacing - 1));

    if (!_cursorBlinking) {
        if (_cursorColor.isValid())
            painter.setPen(_cursorColor);
        else
            painter.setPen(foregroundColor);

        if (_cursorShape == Emulation::KeyboardCursorShape::BlockCursor) {
            // draw the cursor outline, adjusting the area so that
            // it is draw entirely inside 'rect'
            float penWidth = qMax(1, painter.pen().width());

            //
            painter.drawRect(cursorRect.adjusted(+penWidth / 2 + fmod(penWidth, 2),
                                                 +penWidth / 2 + fmod(penWidth, 2),
                                                 -penWidth / 2 - fmod(penWidth, 2),
                                                 -penWidth / 2 - fmod(penWidth, 2)));

            // if ( hasFocus() )
            if (hasActiveFocus()) {
                painter.fillRect(cursorRect, _cursorColor.isValid() ? _cursorColor : foregroundColor);

                if (!_cursorColor.isValid()) {
                    // invert the colour used to draw the text to ensure that the character at
                    // the cursor position is readable
                    invertCharacterColor = true;
                }
            }
        } else if (_cursorShape == Emulation::KeyboardCursorShape::UnderlineCursor)
            painter.drawLine(cursorRect.left(), cursorRect.bottom(), cursorRect.right(), cursorRect.bottom());
        else if (_cursorShape == Emulation::KeyboardCursorShape::IBeamCursor)
            painter.drawLine(cursorRect.left(), cursorRect.top(), cursorRect.left(), cursorRect.bottom());
    }
}

void TerminalDisplay::drawCharacters(QPainter &painter, const QRect &rect, const QString &text, const Character *style, bool invertCharacterColor)
{
    // don't draw text which is currently blinking
    if (_blinking && (style->rendition & RE_BLINK))
        return;

    // don't draw concealed characters
    if (style->rendition & RE_CONCEAL)
        return;

    // setup bold and underline
    bool useBold = ((style->rendition & RE_BOLD) && _boldIntense) || font().bold();
    const bool useUnderline = style->rendition & RE_UNDERLINE || font().underline();
    const bool useItalic = style->rendition & RE_ITALIC || font().italic();
    const bool useStrikeOut = style->rendition & RE_STRIKEOUT || font().strikeOut();
    const bool useOverline = style->rendition & RE_OVERLINE || font().overline();

    painter.setFont(font());

    QFont font = painter.font();
    if (font.bold() != useBold || font.underline() != useUnderline || font.italic() != useItalic || font.strikeOut() != useStrikeOut
        || font.overline() != useOverline) {
        font.setBold(useBold);
        font.setUnderline(useUnderline);
        font.setItalic(useItalic);
        font.setStrikeOut(useStrikeOut);
        font.setOverline(useOverline);
        painter.setFont(font);
    }

    // setup pen
    const CharacterColor &textColor = (invertCharacterColor ? style->backgroundColor : style->foregroundColor);
    const QColor color = textColor.color(_colorTable);
    QPen pen = painter.pen();
    if (pen.color() != color) {
        pen.setColor(color);
        painter.setPen(color);
    }

    // draw text
    if (isLineCharString(text))
        drawLineCharString(painter, rect.x(), rect.y(), text, style);
    else {
        // Force using LTR as the document layout for the terminal area, because
        // there is no use cases for RTL emulator and RTL terminal application.
        //
        // This still allows RTL characters to be rendered in the RTL way.
        painter.setLayoutDirection(Qt::LeftToRight);

        if (_bidiEnabled) {
            painter.drawText(rect.x(), rect.y() + _fontAscent + _lineSpacing, text);
        } else {
            painter.drawText(rect.x(), rect.y() + _fontAscent + _lineSpacing, LTR_OVERRIDE_CHAR + text);
        }
    }
}

void TerminalDisplay::drawTextFragment(QPainter &painter, const QRect &rect, const QString &text, const Character *style)
{
    painter.save();

    // setup painter
    const QColor foregroundColor = style->foregroundColor.color(_colorTable);
    const QColor backgroundColor = style->backgroundColor.color(_colorTable);

    // draw background if different from the display's background color
    if (backgroundColor != palette().window().color())
        drawBackground(painter, rect, backgroundColor, false /* do not use transparency */);

    // draw cursor shape if the current character is the cursor
    // this may alter the foreground and background colors
    bool invertCharacterColor = false;
    if (style->rendition & RE_CURSOR)
        drawCursor(painter, rect, foregroundColor, backgroundColor, invertCharacterColor);

    // draw text
    drawCharacters(painter, rect, text, style, invertCharacterColor);

    painter.restore();
}

void TerminalDisplay::setRandomSeed(uint randomSeed)
{
    _randomSeed = randomSeed;
}
uint TerminalDisplay::randomSeed() const
{
    return _randomSeed;
}

// scrolls the image by 'lines', down if lines > 0 or up otherwise.
//
// the terminal emulation keeps track of the scrolling of the character
// image as it receives input, and when the view is updated, it calls scrollImage()
// with the final scroll amount.  this improves performance because scrolling the
// display is much cheaper than re-rendering all the text for the
// part of the image which has moved up or down.
// Instead only new lines have to be drawn
void TerminalDisplay::scrollImage(int lines, const QRect &screenWindowRegion)
{
    // if the flow control warning is enabled this will interfere with the
    // scrolling optimizations and cause artifacts.  the simple solution here
    // is to just disable the optimization whilst it is visible
    if (_outputSuspendedLabel && _outputSuspendedLabel->isVisible())
        return;

    // constrain the region to the display
    // the bottom of the region is capped to the number of lines in the display's
    // internal image - 2, so that the height of 'region' is strictly less
    // than the height of the internal image.
    QRect region = screenWindowRegion;
    region.setBottom(qMin(region.bottom(), this->_lines - 2));

    // return if there is nothing to do
    if (lines == 0 || _image.empty() || !region.isValid() || (region.top() + abs(lines)) >= region.bottom() || this->_lines <= region.height())
        return;

    // hide terminal size label to prevent it being scrolled
    if (_resizeWidget && _resizeWidget->isVisible())
        _resizeWidget->hide();

    // Note:  With Qt 4.4 the left edge of the scrolled area must be at 0
    // to get the correct (newly exposed) part of the widget repainted.
    //
    // The right edge must be before the left edge of the scroll bar to
    // avoid triggering a repaint of the entire widget, the distance is
    // given by SCROLLBAR_CONTENT_GAP
    //
    // Set the QT_FLUSH_PAINT environment variable to '1' before starting the
    // application to monitor repainting.
    //
    int scrollBarWidth = _scrollBar->isHidden()                                               ? 0
        : _scrollBar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, _scrollBar) ? 0
                                                                                              : _scrollBar->width();
    const int SCROLLBAR_CONTENT_GAP = scrollBarWidth == 0 ? 0 : 1;
    QRect scrollRect;
    if (_scrollbarLocation == QTermWidget::ScrollBarLeft) {
        scrollRect.setLeft(scrollBarWidth + SCROLLBAR_CONTENT_GAP);
        scrollRect.setRight(width());
    } else {
        scrollRect.setLeft(0);
        scrollRect.setRight(width() - scrollBarWidth - SCROLLBAR_CONTENT_GAP);
    }
    void *firstCharPos = &_image[region.top() * this->_columns];
    void *lastCharPos = &_image[(region.top() + abs(lines)) * this->_columns];

    int top = _topMargin + (region.top() * qRound(_fontHeight));
    int linesToMove = region.height() - abs(lines);
    int bytesToMove = linesToMove * this->_columns * sizeof(Character);

    Q_ASSERT(linesToMove > 0);
    Q_ASSERT(bytesToMove > 0);

    // scroll internal image
    if (lines > 0) {
        // check that the memory areas that we are going to move are valid
        Q_ASSERT((char *)lastCharPos + bytesToMove < (char *)(_image.data() + (this->_lines * this->_columns)));

        Q_ASSERT((lines * this->_columns) < _imageSize);

        // scroll internal image down
        memmove(firstCharPos, lastCharPos, bytesToMove);

        // set region of display to scroll
        scrollRect.setTop(top);
    } else {
        // check that the memory areas that we are going to move are valid
        Q_ASSERT((char *)firstCharPos + bytesToMove < (char *)(_image.data() + (this->_lines * this->_columns)));

        // scroll internal image up
        memmove(lastCharPos, firstCharPos, bytesToMove);

        // set region of the display to scroll
        scrollRect.setTop(top + abs(lines) * qRound(_fontHeight));
    }
    scrollRect.setHeight(linesToMove * qRound(_fontHeight));

    Q_ASSERT(scrollRect.isValid() && !scrollRect.isEmpty());

    // scroll the display vertically to match internal _image
    // scroll( 0 , qRound(_fontHeight) * (-lines) , scrollRect );
}

QRegion TerminalDisplay::hotSpotRegion() const
{
    QRegion region;
    const auto hotSpots = _filterChain->hotSpots();
    for (Filter::HotSpot *const hotSpot : hotSpots) {
        QRect r;
        if (hotSpot->startLine() == hotSpot->endLine()) {
            r.setLeft(hotSpot->startColumn());
            r.setTop(hotSpot->startLine());
            r.setRight(hotSpot->endColumn());
            r.setBottom(hotSpot->endLine());
            region |= imageToWidget(r);
            ;
        } else {
            r.setLeft(hotSpot->startColumn());
            r.setTop(hotSpot->startLine());
            r.setRight(_columns);
            r.setBottom(hotSpot->startLine());
            region |= imageToWidget(r);
            ;
            for (int line = hotSpot->startLine() + 1; line < hotSpot->endLine(); line++) {
                r.setLeft(0);
                r.setTop(line);
                r.setRight(_columns);
                r.setBottom(line);
                region |= imageToWidget(r);
                ;
            }
            r.setLeft(0);
            r.setTop(hotSpot->endLine());
            r.setRight(hotSpot->endColumn());
            r.setBottom(hotSpot->endLine());
            region |= imageToWidget(r);
            ;
        }
    }
    return region;
}

void TerminalDisplay::processFilters()
{
    if (!_screenWindow)
        return;

    QRegion preUpdateHotSpots = hotSpotRegion();

    // use _screenWindow->getImage() here rather than _image because
    // other classes may call processFilters() when this display's
    // ScreenWindow emits a scrolled() signal - which will happen before
    // updateImage() is called on the display and therefore _image is
    // out of date at this point
    _filterChain->setImage(_screenWindow->getImage(), _screenWindow->windowLines(), _screenWindow->windowColumns(), _screenWindow->getLineProperties());
    _filterChain->process();

    QRegion postUpdateHotSpots = hotSpotRegion();

    update(preUpdateHotSpots | postUpdateHotSpots);
}

void TerminalDisplay::updateImage()
{
    if (!_screenWindow)
        return;

    // TODO QMLTermWidget at the moment I'm disabling this.
    // Since this can't be scrolled we need to determine if this
    // is useful or not.

    // optimization - scroll the existing image where possible and
    // avoid expensive text drawing for parts of the image that
    // can simply be moved up or down
    //  scrollImage( _screenWindow->scrollCount() ,
    //               _screenWindow->scrollRegion() );
    //  _screenWindow->resetScrollCount();

    if (_image.empty()) {
        // Create _image.
        // The emitted changedContentSizeSignal also leads to getImage being recreated, so do this first.
        updateImageSize();
    }

    auto newimg = _screenWindow->getImage();
    int lines = _screenWindow->windowLines();
    int columns = _screenWindow->windowColumns();

    setScroll(_screenWindow->currentLine(), _screenWindow->lineCount());

    Q_ASSERT(this->_usedLines <= this->_lines);
    Q_ASSERT(this->_usedColumns <= this->_columns);

    int y, x, len;

    QPoint tL = contentsRect().topLeft();
    int tLx = tL.x();
    int tLy = tL.y();
    _hasBlinker = false;

    CharacterColor cf; // undefined
    CharacterColor _clipboard; // undefined
    int cr = -1; // undefined

    const int linesToUpdate = qMin(this->_lines, qMax(0, lines));
    const int columnsToUpdate = qMin(this->_columns, qMax(0, columns));

    std::vector<QChar> disstrU(columnsToUpdate);
    std::vector<char> dirtyMask(columnsToUpdate + 2);
    QRegion dirtyRegion;

    // debugging variable, this records the number of lines that are found to
    // be 'dirty' ( ie. have changed from the old _image to the new _image ) and
    // which therefore need to be repainted
    int dirtyLineCount = 0;

    for (y = 0; y < linesToUpdate; ++y) {
        const Character *currentLine = &_image[y * _columns];
        const Character *const newLine = &newimg[y * columns];

        bool updateLine = false;

        // The dirty mask indicates which characters need repainting. We also
        // mark surrounding neighbours dirty, in case the character exceeds
        // its cell boundaries
        memset(dirtyMask.data(), 0, columnsToUpdate + 2);

        for (x = 0; x < columnsToUpdate; ++x) {
            if (newLine[x] != currentLine[x]) {
                dirtyMask[x] = true;
            }
        }

        if (!_resizing) // not while _resizing, we're expecting a paintEvent
            for (x = 0; x < columnsToUpdate; ++x) {
                _hasBlinker |= (newLine[x].rendition & RE_BLINK);

                // Start drawing if this character or the next one differs.
                // We also take the next one into account to handle the situation
                // where characters exceed their cell width.
                if (dirtyMask[x]) {
                    QChar c = newLine[x + 0].character;
                    if (!c.unicode())
                        continue;
                    int p = 0;
                    disstrU[p++] = c; // fontMap(c);
                    bool lineDraw = isLineChar(c);
                    bool doubleWidth = (x + 1 == columnsToUpdate) ? false : (newLine[x + 1].character.unicode() == 0);
                    cr = newLine[x].rendition;
                    _clipboard = newLine[x].backgroundColor;
                    if (newLine[x].foregroundColor != cf)
                        cf = newLine[x].foregroundColor;
                    int lln = columnsToUpdate - x;
                    for (len = 1; len < lln; ++len) {
                        const Character &ch = newLine[x + len];

                        if (!ch.character.unicode())
                            continue; // Skip trailing part of multi-col chars.

                        bool nextIsDoubleWidth = (x + len + 1 == columnsToUpdate) ? false : (newLine[x + len + 1].character.unicode() == 0);

                        if (ch.foregroundColor != cf || ch.backgroundColor != _clipboard || ch.rendition != cr || !dirtyMask[x + len]
                            || isLineChar(c) != lineDraw || nextIsDoubleWidth != doubleWidth)
                            break;

                        disstrU[p++] = c; // fontMap(c);
                    }

                    bool saveFixedFont = _fixedFont;
                    if (lineDraw)
                        _fixedFont = false;
                    if (doubleWidth)
                        _fixedFont = false;

                    updateLine = true;

                    _fixedFont = saveFixedFont;
                    x += len - 1;
                }
            }

        // both the top and bottom halves of double height _lines must always be redrawn
        // although both top and bottom halves contain the same characters, only
        // the top one is actually
        // drawn.
        if (_lineProperties.size() > y)
            updateLine |= (_lineProperties[y] & LINE_DOUBLEHEIGHT);

        // if the characters on the line are different in the old and the new _image
        // then this line must be repainted.
        if (updateLine) {
            dirtyLineCount++;

            // add the area occupied by this line to the region which needs to be
            // repainted
            QRect dirtyRect = QRect(_leftMargin + tLx, _topMargin + tLy + qRound(_fontHeight) * y, _fontWidth * columnsToUpdate, qRound(_fontHeight));

            dirtyRegion |= dirtyRect;
        }

        // replace the line of characters in the old _image with the
        // current line of the new _image
        memcpy((void *)currentLine, (const void *)newLine, columnsToUpdate * sizeof(Character));
    }

    // if the new _image is smaller than the previous _image, then ensure that the area
    // outside the new _image is cleared
    if (linesToUpdate < _usedLines) {
        dirtyRegion |= QRect(_leftMargin + tLx,
                             _topMargin + tLy + qRound(_fontHeight) * linesToUpdate,
                             _fontWidth * this->_columns,
                             qRound(_fontHeight) * (_usedLines - linesToUpdate));
    }
    _usedLines = linesToUpdate;

    if (columnsToUpdate < _usedColumns) {
        dirtyRegion |= QRect(_leftMargin + tLx + columnsToUpdate * _fontWidth,
                             _topMargin + tLy,
                             _fontWidth * (_usedColumns - columnsToUpdate),
                             qRound(_fontHeight) * this->_lines);
    }
    _usedColumns = columnsToUpdate;

    dirtyRegion |= _inputMethodData.previousPreeditRect;

    // update the parts of the display which have changed
    update(dirtyRegion);

    if (_hasBlinker && !_blinkTimer->isActive())
        _blinkTimer->start(TEXT_BLINK_DELAY);
    if (!_hasBlinker && _blinkTimer->isActive()) {
        _blinkTimer->stop();
        _blinking = false;
    }
}

void TerminalDisplay::showResizeNotification()
{
}

void TerminalDisplay::setBlinkingCursor(bool blink)
{
    if (_hasBlinkingCursor != blink)
        Q_EMIT blinkingCursorStateChanged();

    _hasBlinkingCursor = blink;

    if (blink && !_blinkCursorTimer->isActive())
        _blinkCursorTimer->start(QApplication::cursorFlashTime() / 2);

    if (!blink && _blinkCursorTimer->isActive()) {
        _blinkCursorTimer->stop();
        if (_cursorBlinking)
            blinkCursorEvent();
        else
            _cursorBlinking = false;
    }
}

void TerminalDisplay::setBlinkingTextEnabled(bool blink)
{
    _allowBlinkingText = blink;

    if (blink && !_blinkTimer->isActive())
        _blinkTimer->start(TEXT_BLINK_DELAY);

    if (!blink && _blinkTimer->isActive()) {
        _blinkTimer->stop();
        _blinking = false;
    }
}

void TerminalDisplay::focusOutEvent(QFocusEvent *)
{
    Q_EMIT termLostFocus();
    // trigger a repaint of the cursor so that it is both visible (in case
    // it was hidden during blinking)
    // and drawn in a focused out state
    _cursorBlinking = false;
    updateCursor();

    _blinkCursorTimer->stop();
    if (_blinking)
        blinkEvent();

    _blinkTimer->stop();
}
void TerminalDisplay::focusInEvent(QFocusEvent *)
{
    Q_EMIT termGetFocus();
    if (_hasBlinkingCursor) {
        _blinkCursorTimer->start();
    }
    updateCursor();

    if (_hasBlinker)
        _blinkTimer->start();
}

// QMLTermWidget version. See the upstream commented version for reference.
void TerminalDisplay::paint(QPainter *painter)
{
    QRect clipRect = painter->clipBoundingRect().toAlignedRect();
    QRect dirtyRect = clipRect.isValid() ? clipRect : contentsRect();
    drawContents(*painter, dirtyRect);
}

QPoint TerminalDisplay::cursorPosition() const
{
    if (_screenWindow)
        return _screenWindow->cursorPosition();
    else
        return {0, 0};
}

QRect TerminalDisplay::preeditRect() const
{
    const int preeditLength = string_width(_inputMethodData.preeditString);

    if (preeditLength == 0)
        return {};

    return QRect(_leftMargin + qRound(_fontWidth) * cursorPosition().x(),
                 _topMargin + qRound(_fontHeight) * cursorPosition().y(),
                 qRound(_fontWidth) * preeditLength,
                 qRound(_fontHeight));
}

void TerminalDisplay::drawInputMethodPreeditString(QPainter &painter, const QRect &rect)
{
    if (_inputMethodData.preeditString.isEmpty())
        return;

    const QPoint cursorPos = cursorPosition();

    bool invertColors = false;
    const QColor background = _colorTable[DEFAULT_BACK_COLOR].color;
    const QColor foreground = _colorTable[DEFAULT_FORE_COLOR].color;
    const Character *style = &_image[loc(cursorPos.x(), cursorPos.y())];

    drawBackground(painter, rect, background, true);
    drawCursor(painter, rect, foreground, background, invertColors);
    drawCharacters(painter, rect, _inputMethodData.preeditString, style, invertColors);

    _inputMethodData.previousPreeditRect = rect;
}

FilterChain *TerminalDisplay::filterChain() const
{
    return _filterChain.get();
}

void TerminalDisplay::paintFilters(QPainter &painter)
{
    // get color of character under mouse and use it to draw
    // lines for filters
    QPoint cursorPos = mapFromScene(QCursor::pos()).toPoint();
    int leftMargin = _leftBaseMargin
        + ((_scrollbarLocation == QTermWidget::ScrollBarLeft && !_scrollBar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, _scrollBar))
               ? _scrollBar->width()
               : 0);

    auto charPos = getCharacterPosition(cursorPos);
    Character cursorCharacter = _image[loc(charPos.columns, charPos.lines)];

    painter.setPen(QPen(cursorCharacter.foregroundColor.color(colorTable())));

    // iterate over hotspots identified by the display's currently active filters
    // and draw appropriate visuals to indicate the presence of the hotspot

    const QList<Filter::HotSpot *> spots = _filterChain->hotSpots();
    for (const auto spot : spots) {
        QRegion region;
        if (spot->type() == Filter::HotSpot::Link) {
            QRect r;
            if (spot->startLine() == spot->endLine()) {
                r.setCoords(spot->startColumn() * qRound(_fontWidth) + 1 + leftMargin,
                            spot->startLine() * qRound(_fontHeight) + 1 + _topBaseMargin,
                            spot->endColumn() * qRound(_fontWidth) - 1 + leftMargin,
                            (spot->endLine() + 1) * qRound(_fontHeight) - 1 + _topBaseMargin);
                region |= r;
            } else {
                r.setCoords(spot->startColumn() * qRound(_fontWidth) + 1 + leftMargin,
                            spot->startLine() * qRound(_fontHeight) + 1 + _topBaseMargin,
                            _columns * qRound(_fontWidth) - 1 + leftMargin,
                            (spot->startLine() + 1) * qRound(_fontHeight) - 1 + _topBaseMargin);
                region |= r;
                for (int line = spot->startLine() + 1; line < spot->endLine(); line++) {
                    r.setCoords(0 * qRound(_fontWidth) + 1 + leftMargin,
                                line * qRound(_fontHeight) + 1 + _topBaseMargin,
                                _columns * qRound(_fontWidth) - 1 + leftMargin,
                                (line + 1) * qRound(_fontHeight) - 1 + _topBaseMargin);
                    region |= r;
                }
                r.setCoords(0 * qRound(_fontWidth) + 1 + leftMargin,
                            spot->endLine() * qRound(_fontHeight) + 1 + _topBaseMargin,
                            spot->endColumn() * qRound(_fontWidth) - 1 + leftMargin,
                            (spot->endLine() + 1) * qRound(_fontHeight) - 1 + _topBaseMargin);
                region |= r;
            }
        }

        for (int line = spot->startLine(); line <= spot->endLine(); line++) {
            int startColumn = 0;
            int endColumn = _columns - 1; // TODO use number of _columns which are actually
                                          // occupied on this line rather than the width of the
                                          // display in _columns

            // ignore whitespace at the end of the lines
            while (QChar(_image[loc(endColumn, line)].character).isSpace() && endColumn > 0)
                endColumn--;

            // increment here because the column which we want to set 'endColumn' to
            // is the first whitespace character at the end of the line
            endColumn++;

            if (line == spot->startLine())
                startColumn = spot->startColumn();
            if (line == spot->endLine())
                endColumn = spot->endColumn();

            // subtract one pixel from
            // the right and bottom so that
            // we do not overdraw adjacent
            // hotspots
            //
            // subtracting one pixel from all sides also prevents an edge case where
            // moving the mouse outside a link could still leave it underlined
            // because the check below for the position of the cursor
            // finds it on the border of the target area
            QRect r;
            r.setCoords(startColumn * qRound(_fontWidth) + 1 + leftMargin,
                        line * qRound(_fontHeight) + 1 + _topBaseMargin,
                        endColumn * qRound(_fontWidth) - 1 + leftMargin,
                        (line + 1) * qRound(_fontHeight) - 1 + _topBaseMargin);
            // Underline link hotspots
            if (spot->type() == Filter::HotSpot::Link) {
                QFontMetricsF metrics(font());

                // find the baseline (which is the invisible line that the characters in the font sit on,
                // with some having tails dangling below)
                qreal baseline = (qreal)r.bottom() - metrics.descent();
                // find the position of the underline below that
                qreal underlinePos = baseline + metrics.underlinePos();

                if (region.contains(mapFromScene(QCursor::pos()).toPoint())) {
                    painter.drawLine(r.left(), underlinePos, r.right(), underlinePos);
                }
            }
            // Marker hotspots simply have a transparent rectanglular shape
            // drawn on top of them
            else if (spot->type() == Filter::HotSpot::Marker) {
                // TODO - Do not use a hardcoded colour for this
                painter.fillRect(r, QBrush(QColor(255, 0, 0, 120)));
            }
        }
    }
}

int TerminalDisplay::textWidth(const int startColumn, const int length, const int line) const
{
    QFontMetricsF fm(font());
    qreal result = 0;
    for (int column = 0; column < length; column++) {
        result += fm.horizontalAdvance(_image[loc(startColumn + column, line)].character);
    }
    return result;
}

QRect TerminalDisplay::calculateTextArea(int topLeftX, int topLeftY, int startColumn, int line, int length)
{
    int left = _fixedFont ? qRound(_fontWidth) * startColumn : textWidth(0, startColumn, line);
    int top = qRound(_fontHeight) * line;
    int width = _fixedFont ? qRound(_fontWidth) * length : textWidth(startColumn, length, line);
    return {_leftMargin + topLeftX + left, _topMargin + topLeftY + top, width, qRound(_fontHeight)};
}

void TerminalDisplay::drawContents(QPainter &paint, const QRect &rect)
{
    // Draw opaque background
    drawBackground(paint, contentsRect(), _colorTable[DEFAULT_BACK_COLOR].color, true);

    QPoint tL = contentsRect().topLeft();
    int tLx = tL.x();
    int tLy = tL.y();

    int lux = qMin(_usedColumns - 1, qMax(0, qRound((rect.left() - tLx - _leftMargin) / _fontWidth)));
    int luy = qMin(_usedLines - 1, qMax(0, qRound((rect.top() - tLy - _topMargin) / _fontHeight)));
    int rlx = qMin(_usedColumns - 1, qMax(0, qRound((rect.right() - tLx - _leftMargin) / _fontWidth)));
    int rly = qMin(_usedLines - 1, qMax(0, qRound((rect.bottom() - tLy - _topMargin) / _fontHeight)));

    if (_image.empty()) {
        return;
    }

    const int bufferSize = _usedColumns;
    QString unistr;
    unistr.reserve(bufferSize);
    for (int y = luy; y <= rly; y++) {
        char16_t c = _image[loc(lux, y)].character.unicode();
        int x = lux;
        if (!c && x)
            x--; // Search for start of multi-column character
        for (; x <= rlx; x++) {
            int len = 1;
            int p = 0;

            // reset our buffer to the maximal size
            unistr.resize(bufferSize);

            // is this a single character or a sequence of characters ?
            if (_image[loc(x, y)].rendition & RE_EXTENDED_CHAR) {
                // sequence of characters
                ushort extendedCharLength = 0;
                std::span chars = ExtendedCharTable::instance.lookupExtendedChar(_image[loc(x, y)].charSequence, extendedCharLength);
                for (int index = 0; index < extendedCharLength; index++) {
                    Q_ASSERT(p < bufferSize);
                    unistr[p++] = chars[index];
                }
            } else {
                // single character
                c = _image[loc(x, y)].character.unicode();
                if (c) {
                    Q_ASSERT(p < bufferSize);
                    unistr[p++] = c; // fontMap(c);
                }
            }

            bool lineDraw = isLineChar(c);
            bool doubleWidth = (_image[qMin(loc(x, y) + 1, _imageSize)].character.unicode() == 0);
            CharacterColor currentForeground = _image[loc(x, y)].foregroundColor;
            CharacterColor currentBackground = _image[loc(x, y)].backgroundColor;
            quint8 currentRendition = _image[loc(x, y)].rendition;

            while (x + len <= rlx && _image[loc(x + len, y)].foregroundColor == currentForeground
                   && _image[loc(x + len, y)].backgroundColor == currentBackground && _image[loc(x + len, y)].rendition == currentRendition
                   && (_image[qMin(loc(x + len, y) + 1, _imageSize)].character.unicode() == 0) == doubleWidth
                   && isLineChar(c = _image[loc(x + len, y)].character.unicode()) == lineDraw) // Assignment!
            {
                if (c)
                    unistr[p++] = c; // fontMap(c);
                if (doubleWidth) // assert((_image[loc(x+len,y)+1].character == 0)), see above if condition
                    len++; // Skip trailing part of multi-column character
                len++;
            }
            if ((x + len < _usedColumns) && (!_image[loc(x + len, y)].character.unicode()))
                len++; // Adjust for trailing part of multi-column character

            bool save__fixedFont = _fixedFont;
            if (lineDraw)
                _fixedFont = false;
            unistr.resize(p);

            // Create a text scaling matrix for double width and double height lines.
            QTransform textScale;

            if (y < _lineProperties.size()) {
                if (_lineProperties[y] & LINE_DOUBLEWIDTH)
                    textScale.scale(2, 1);

                if (_lineProperties[y] & LINE_DOUBLEHEIGHT)
                    textScale.scale(1, 2);
            }

            // Apply text scaling matrix.
            paint.setWorldTransform(textScale, true);

            // calculate the area in which the text will be drawn
            QRect textArea = calculateTextArea(tLx, tLy, x, y, len);

            // move the calculated area to take account of scaling applied to the painter.
            // the position of the area from the origin (0,0) is scaled
            // by the opposite of whatever
            // transformation has been applied to the painter.  this ensures that
            // painting does actually start from textArea.topLeft()
            //(instead of textArea.topLeft() * painter-scale)
            textArea.moveTopLeft(textScale.inverted().map(textArea.topLeft()));

            // paint text fragment
            drawTextFragment(paint, textArea, unistr, &_image[loc(x, y)]); //,
            // 0,
            //!_isPrinting );

            _fixedFont = save__fixedFont;

            // reset back to single-width, single-height _lines
            paint.setWorldTransform(textScale.inverted(), true);

            if (y < _lineProperties.size() - 1) {
                // double-height _lines are represented by two adjacent _lines
                // containing the same characters
                // both _lines will have the LINE_DOUBLEHEIGHT attribute.
                // If the current line has the LINE_DOUBLEHEIGHT attribute,
                // we can therefore skip the next line
                if (_lineProperties[y] & LINE_DOUBLEHEIGHT)
                    y++;
            }

            x += len - 1;
        }
    }
}

void TerminalDisplay::blinkEvent()
{
    if (!_allowBlinkingText)
        return;

    _blinking = !_blinking;

    // TODO:  Optimize to only repaint the areas of the widget
    //  where there is blinking text
    //  rather than repainting the whole widget.
    update();
}

QRect TerminalDisplay::imageToWidget(const QRect &imageArea) const
{
    QRect result;
    result.setLeft(_leftMargin + qRound(_fontWidth) * imageArea.left());
    result.setTop(_topMargin + qRound(_fontHeight) * imageArea.top());
    result.setWidth(qRound(_fontWidth) * imageArea.width());
    result.setHeight(qRound(_fontHeight) * imageArea.height());

    return result;
}

void TerminalDisplay::updateCursor()
{
    QRect cursorRect = imageToWidget(QRect(cursorPosition(), QSize(1, 1)));
    update(cursorRect);
}

void TerminalDisplay::blinkCursorEvent()
{
    _cursorBlinking = !_cursorBlinking;
    updateCursor();
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                  Resizing                                 */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void TerminalDisplay::resizeEvent(QResizeEvent *)
{
    updateImageSize();
    processFilters();
}

void TerminalDisplay::propagateSize()
{
    if (_isFixedSize) {
        setSize(_columns, _lines);
        return;
    }
    if (!_image.empty())
        updateImageSize();
}

void TerminalDisplay::updateImageSize()
{
    auto oldimg = _image;
    int oldlin = _lines;
    int oldcol = _columns;

    makeImage();

    // copy the old image to reduce flicker
    int lines = qMin(oldlin, _lines);
    int columns = qMin(oldcol, _columns);

    if (!oldimg.empty()) {
        for (int line = 0; line < lines; line++) {
            memcpy((void *)&_image[_columns * line], (void *)&oldimg[oldcol * line], columns * sizeof(Character));
        }
        oldimg.clear();
    }

    if (_screenWindow)
        _screenWindow->setWindowLines(_lines);

    _resizing = (oldlin != _lines) || (oldcol != _columns);

    if (_resizing) {
        showResizeNotification();
        Q_EMIT changedContentSizeSignal(_contentHeight, _contentWidth); // expose resizeEvent
    }

    _resizing = false;
}

// showEvent and hideEvent are reimplemented here so that it appears to other classes that the
// display has been resized when the display is hidden or shown.
//
// TODO: Perhaps it would be better to have separate signals for show and hide instead of using
// the same signal as the one for a content size change
void TerminalDisplay::showEvent(QShowEvent *)
{
    Q_EMIT changedContentSizeSignal(_contentHeight, _contentWidth);
}
void TerminalDisplay::hideEvent(QHideEvent *)
{
    Q_EMIT changedContentSizeSignal(_contentHeight, _contentWidth);
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                Scrollbar                                  */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void TerminalDisplay::scrollBarPositionChanged(int)
{
    if (!_screenWindow)
        return;

    _screenWindow->scrollTo(_scrollBar->value());

    // if the thumb has been moved to the bottom of the _scrollBar then set
    // the display to automatically track new output,
    // that is, scroll down automatically
    // to how new _lines as they are added
    const bool atEndOfOutput = (_scrollBar->value() == _scrollBar->maximum());
    _screenWindow->setTrackOutput(atEndOfOutput);

    updateImage();

    // QMLTermWidget: notify qml side of the change only when needed.
    Q_EMIT scrollbarValueChanged();
}

void TerminalDisplay::setScroll(int cursor, int slines)
{
    // update _scrollBar if the range or value has changed,
    // otherwise return
    //
    // setting the range or value of a _scrollBar will always trigger
    // a repaint, so it should be avoided if it is not necessary
    if (_scrollBar->minimum() == 0 && _scrollBar->maximum() == (slines - _lines) && _scrollBar->value() == cursor) {
        return;
    }

    disconnect(_scrollBar, &QAbstractSlider::valueChanged, this, &TerminalDisplay::scrollBarPositionChanged);
    _scrollBar->setRange(0, slines - _lines);
    _scrollBar->setSingleStep(1);
    _scrollBar->setPageStep(_lines);
    _scrollBar->setValue(cursor);
    connect(_scrollBar, &QAbstractSlider::valueChanged, this, &TerminalDisplay::scrollBarPositionChanged);
}

void TerminalDisplay::scrollToEnd()
{
    disconnect(_scrollBar, &QAbstractSlider::valueChanged, this, &TerminalDisplay::scrollBarPositionChanged);
    _scrollBar->setValue(_scrollBar->maximum());
    connect(_scrollBar, &QAbstractSlider::valueChanged, this, &TerminalDisplay::scrollBarPositionChanged);

    _screenWindow->scrollTo(_scrollBar->value() + 1);
    _screenWindow->setTrackOutput(_screenWindow->atEndOfOutput());
}

void TerminalDisplay::setScrollBarPosition(QTermWidget::ScrollBarPosition position)
{
    if (_scrollbarLocation == position)
        return;

    if (position == QTermWidget::NoScrollBar)
        _scrollBar->hide();
    else
        _scrollBar->show();

    _topMargin = _leftMargin = 1;
    _scrollbarLocation = position;

    propagateSize();
    update();
}

void TerminalDisplay::mousePressEvent(QMouseEvent *ev)
{
    if (_possibleTripleClick && (ev->button() == Qt::LeftButton)) {
        mouseTripleClickEvent(ev);
        return;
    }

    if (!contentsRect().contains(ev->pos()))
        return;

    if (!_screenWindow)
        return;

    auto charPos = getCharacterPosition(ev->pos());
    QPoint pos = charPos;
    auto [charColumn, charLine] = charPos;

    if (ev->button() == Qt::LeftButton) {
        _lineSelectionMode = false;
        _wordSelectionMode = false;

        Q_EMIT isBusySelecting(true); // Keep it steady...
        // Drag only when the Control key is hold
        bool selected = false;

        // The receiver of the testIsSelected() signal will adjust
        // 'selected' accordingly.
        // emit testIsSelected(pos.x(), pos.y(), selected);

        selected = _screenWindow->isSelected(charColumn, charLine);

        if ((!_ctrlDrag || ev->modifiers() & Qt::ControlModifier) && selected) {
            // The user clicked inside selected text
            dragInfo.state = diPending;
            dragInfo.start = ev->pos();
        } else {
            // No reason to ever start a drag event
            dragInfo.state = diNone;

            _preserveLineBreaks = !((ev->modifiers() & Qt::ControlModifier) && !(ev->modifiers() & Qt::AltModifier));
            _columnSelectionMode = (ev->modifiers() & Qt::AltModifier) && (ev->modifiers() & Qt::ControlModifier);

            if (_mouseMarks || (ev->modifiers() & Qt::ShiftModifier)) {
                _screenWindow->clearSelection();

                // emit clearSelectionSignal();
                pos.ry() += _scrollBar->value();
                _iPntSel = _pntSel = pos;
                _actSel = 1; // left mouse button pressed but nothing selected yet.

            } else {
                Q_EMIT mouseSignal(0, charColumn + 1, charLine + 1 + _scrollBar->value() - _scrollBar->maximum(), 0);
            }

            Filter::HotSpot *spot = _filterChain->hotSpotAt(charLine, charColumn);
            if (spot && spot->type() == Filter::HotSpot::Link)
                spot->activate(QLatin1String("click-action"));
        }
    } else if (ev->button() == Qt::MiddleButton) {
        if (_mouseMarks || (ev->modifiers() & Qt::ShiftModifier))
            emitSelection(true, ev->modifiers() & Qt::ControlModifier);
        else
            Q_EMIT mouseSignal(1, charColumn + 1, charLine + 1 + _scrollBar->value() - _scrollBar->maximum(), 0);
    } else if (ev->button() == Qt::RightButton) {
        if (_mouseMarks || (ev->modifiers() & Qt::ShiftModifier))
            Q_EMIT configureRequest(ev->pos());
        else
            Q_EMIT mouseSignal(2, charColumn + 1, charLine + 1 + _scrollBar->value() - _scrollBar->maximum(), 0);
    }
}

QList<QAction *> TerminalDisplay::filterActions(const QPoint &position)
{
    auto pos = getCharacterPosition(position);

    Filter::HotSpot *spot = _filterChain->hotSpotAt(pos.lines, pos.columns);

    return spot ? spot->actions() : QList<QAction *>();
}

void TerminalDisplay::mouseMoveEvent(QMouseEvent *ev)
{
    int leftMargin = _leftBaseMargin
        + ((_scrollbarLocation == QTermWidget::ScrollBarLeft && !_scrollBar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, _scrollBar))
               ? _scrollBar->width()
               : 0);

    auto charPos = getCharacterPosition(ev->pos());

    // handle filters
    // change link hot-spot appearance on mouse-over
    Filter::HotSpot *spot = _filterChain->hotSpotAt(charPos.lines, charPos.columns);
    if (spot && spot->type() == Filter::HotSpot::Link) {
        QRegion previousHotspotArea = _mouseOverHotspotArea;
        _mouseOverHotspotArea = QRegion();
        QRect r;
        if (spot->startLine() == spot->endLine()) {
            r.setCoords(spot->startColumn() * qRound(_fontWidth) + leftMargin,
                        spot->startLine() * qRound(_fontHeight) + _topBaseMargin,
                        spot->endColumn() * qRound(_fontWidth) + leftMargin,
                        (spot->endLine() + 1) * qRound(_fontHeight) - 1 + _topBaseMargin);
            _mouseOverHotspotArea |= r;
        } else {
            r.setCoords(spot->startColumn() * qRound(_fontWidth) + leftMargin,
                        spot->startLine() * qRound(_fontHeight) + _topBaseMargin,
                        _columns * qRound(_fontWidth) - 1 + leftMargin,
                        (spot->startLine() + 1) * qRound(_fontHeight) + _topBaseMargin);
            _mouseOverHotspotArea |= r;
            for (int line = spot->startLine() + 1; line < spot->endLine(); line++) {
                r.setCoords(0 * qRound(_fontWidth) + leftMargin,
                            line * qRound(_fontHeight) + _topBaseMargin,
                            _columns * qRound(_fontWidth) + leftMargin,
                            (line + 1) * qRound(_fontHeight) + _topBaseMargin);
                _mouseOverHotspotArea |= r;
            }
            r.setCoords(0 * qRound(_fontWidth) + leftMargin,
                        spot->endLine() * qRound(_fontHeight) + _topBaseMargin,
                        spot->endColumn() * qRound(_fontWidth) + leftMargin,
                        (spot->endLine() + 1) * qRound(_fontHeight) + _topBaseMargin);
            _mouseOverHotspotArea |= r;
        }
        update(_mouseOverHotspotArea | previousHotspotArea);
    } else if (!_mouseOverHotspotArea.isEmpty()) {
        update(_mouseOverHotspotArea);
        // set hotspot area to an invalid rectangle
        _mouseOverHotspotArea = QRegion();
    }

    // for auto-hiding the cursor, we need mouseTracking
    if (ev->buttons() == Qt::NoButton)
        return;

    // if the terminal is interested in mouse movements
    // then Q_EMIT a mouse movement signal, unless the shift
    // key is being held down, which overrides this.
    if (!_mouseMarks && !(ev->modifiers() & Qt::ShiftModifier)) {
        int button = 3;
        if (ev->buttons() & Qt::LeftButton)
            button = 0;
        if (ev->buttons() & Qt::MiddleButton)
            button = 1;
        if (ev->buttons() & Qt::RightButton)
            button = 2;

        Q_EMIT mouseSignal(button, charPos.columns + 1, charPos.lines + 1 + _scrollBar->value() - _scrollBar->maximum(), 1);

        return;
    }

    if (dragInfo.state == diPending) {
        // we had a mouse down, but haven't confirmed a drag yet
        // if the mouse has moved sufficiently, we will confirm

        //   int distance = KGlobalSettings::dndEventDelay();
        int distance = QApplication::startDragDistance();
        if (ev->position().x() > dragInfo.start.x() + distance || ev->position().x() < dragInfo.start.x() - distance
            || ev->position().y() > dragInfo.start.y() + distance || ev->position().y() < dragInfo.start.y() - distance) {
            // we've left the drag square, we can start a real drag operation now
            Q_EMIT isBusySelecting(false); // Ok.. we can breath again.

            _screenWindow->clearSelection();
            doDrag();
        }
        return;
    } else if (dragInfo.state == diDragging) {
        // this isn't technically needed because mouseMoveEvent is suppressed during
        // Qt drag operations, replaced by dragMoveEvent
        return;
    }

    if (_actSel == 0)
        return;

    // don't extend selection while pasting
    if (ev->buttons() & Qt::MiddleButton)
        return;

    extendSelection(ev->pos());
}

void TerminalDisplay::extendSelection(const QPoint &position)
{
    QPoint pos = position;

    if (!_screenWindow)
        return;

    // if ( !contentsRect().contains(ev->pos()) ) return;
    QPoint tL = contentsRect().topLeft();
    int tLx = tL.x();
    int tLy = tL.y();
    int scroll = _scrollBar->value();

    // we're in the process of moving the mouse with the left button pressed
    // the mouse cursor will kept caught within the bounds of the text in
    // this widget.

    int linesBeyondWidget = 0;

    QRect textBounds(tLx + _leftMargin, tLy + _topMargin, _usedColumns * qRound(_fontWidth) - 1, _usedLines * qRound(_fontHeight) - 1);

    // Adjust position within text area bounds.
    QPoint oldpos = pos;

    pos.setX(qBound(textBounds.left(), pos.x(), textBounds.right()));
    pos.setY(qBound(textBounds.top(), pos.y(), textBounds.bottom()));

    if (oldpos.y() > textBounds.bottom()) {
        linesBeyondWidget = (oldpos.y() - textBounds.bottom()) / qRound(_fontHeight);
        _scrollBar->setValue(_scrollBar->value() + linesBeyondWidget + 1); // scrollforward
    }
    if (oldpos.y() < textBounds.top()) {
        linesBeyondWidget = (textBounds.top() - oldpos.y()) / qRound(_fontHeight);
        _scrollBar->setValue(_scrollBar->value() - linesBeyondWidget - 1); // history
    }

    QPoint here =
        getCharacterPosition(pos); // QPoint((pos.x()-tLx-_leftMargin+(qRound(_fontWidth)/2))/qRound(_fontWidth),(pos.y()-tLy-_topMargin)/qRound(_fontHeight));
    QPoint ohere;
    QPoint _iPntSelCorr = _iPntSel;
    _iPntSelCorr.ry() -= _scrollBar->value();
    QPoint _pntSelCorr = _pntSel;
    _pntSelCorr.ry() -= _scrollBar->value();
    bool swapping = false;

    if (_wordSelectionMode) {
        // Extend to word boundaries
        int i;
        QChar selClass;

        bool left_not_right = (here.y() < _iPntSelCorr.y() || (here.y() == _iPntSelCorr.y() && here.x() < _iPntSelCorr.x()));
        bool old_left_not_right = (_pntSelCorr.y() < _iPntSelCorr.y() || (_pntSelCorr.y() == _iPntSelCorr.y() && _pntSelCorr.x() < _iPntSelCorr.x()));
        swapping = left_not_right != old_left_not_right;

        // Find left (left_not_right ? from here : from start)
        QPoint left = left_not_right ? here : _iPntSelCorr;
        i = loc(left.x(), left.y());
        if (i >= 0 && i <= _imageSize) {
            selClass = charClass(_image[i].character);
            while (((left.x() > 0) || (left.y() > 0 && (_lineProperties[left.y() - 1] & LINE_WRAPPED))) && charClass(_image[i - 1].character) == selClass) {
                i--;
                if (left.x() > 0)
                    left.rx()--;
                else {
                    left.rx() = _usedColumns - 1;
                    left.ry()--;
                }
            }
        }

        // Find left (left_not_right ? from start : from here)
        QPoint right = left_not_right ? _iPntSelCorr : here;
        i = loc(right.x(), right.y());
        if (i >= 0 && i <= _imageSize) {
            selClass = charClass(_image[i].character);
            while (((right.x() < _usedColumns - 1) || (right.y() < _usedLines - 1 && (_lineProperties[right.y()] & LINE_WRAPPED)))
                   && charClass(_image[i + 1].character) == selClass) {
                i++;
                if (right.x() < _usedColumns - 1)
                    right.rx()++;
                else {
                    right.rx() = 0;
                    right.ry()++;
                }
            }
        }

        // Pick which is start (ohere) and which is extension (here)
        if (left_not_right) {
            here = left;
            ohere = right;
        } else {
            here = right;
            ohere = left;
        }
        ohere.rx()++;
    }

    if (_lineSelectionMode) {
        // Extend to complete line
        bool above_not_below = (here.y() < _iPntSelCorr.y());

        QPoint above = above_not_below ? here : _iPntSelCorr;
        QPoint below = above_not_below ? _iPntSelCorr : here;

        while (above.y() > 0 && (_lineProperties[above.y() - 1] & LINE_WRAPPED))
            above.ry()--;
        while (below.y() < _usedLines - 1 && (_lineProperties[below.y()] & LINE_WRAPPED))
            below.ry()++;

        above.setX(0);
        below.setX(_usedColumns - 1);

        // Pick which is start (ohere) and which is extension (here)
        if (above_not_below) {
            here = above;
            ohere = below;
        } else {
            here = below;
            ohere = above;
        }

        QPoint newSelBegin = QPoint(ohere.x(), ohere.y());
        swapping = !(_tripleSelBegin == newSelBegin);
        _tripleSelBegin = newSelBegin;

        ohere.rx()++;
    }

    int offset = 0;
    if (!_wordSelectionMode && !_lineSelectionMode) {
        int i;
        QChar selClass;

        bool left_not_right = (here.y() < _iPntSelCorr.y() || (here.y() == _iPntSelCorr.y() && here.x() < _iPntSelCorr.x()));
        bool old_left_not_right = (_pntSelCorr.y() < _iPntSelCorr.y() || (_pntSelCorr.y() == _iPntSelCorr.y() && _pntSelCorr.x() < _iPntSelCorr.x()));
        swapping = left_not_right != old_left_not_right;

        // Find left (left_not_right ? from here : from start)
        QPoint left = left_not_right ? here : _iPntSelCorr;

        // Find left (left_not_right ? from start : from here)
        QPoint right = left_not_right ? _iPntSelCorr : here;
        if (right.x() > 0 && !_columnSelectionMode) {
            i = loc(right.x(), right.y());
            if (i >= 0 && i <= _imageSize) {
                selClass = charClass(_image[i - 1].character);
                /* if (selClass == ' ')
                 {
                   while ( right.x() < _usedColumns-1 && charClass(_image[i+1].character) == selClass && (right.y()<_usedLines-1) &&
                                   !(_lineProperties[right.y()] & LINE_WRAPPED))
                   { i++; right.rx()++; }
                   if (right.x() < _usedColumns-1)
                     right = left_not_right ? _iPntSelCorr : here;
                   else
                     right.rx()++;  // will be balanced later because of offset=-1;
                 }*/
            }
        }

        // Pick which is start (ohere) and which is extension (here)
        if (left_not_right) {
            here = left;
            ohere = right;
            offset = 0;
        } else {
            here = right;
            ohere = left;
            offset = -1;
        }
    }

    if ((here == _pntSelCorr) && (scroll == _scrollBar->value()))
        return; // not moved

    if (here == ohere)
        return; // It's not left, it's not right.

    if (_actSel < 2 || swapping) {
        if (_columnSelectionMode && !_lineSelectionMode && !_wordSelectionMode) {
            _screenWindow->setSelectionStart(ohere.x(), ohere.y(), true);
        } else {
            _screenWindow->setSelectionStart(ohere.x() - 1 - offset, ohere.y(), false);
        }
    }

    _actSel = 2; // within selection
    _pntSel = here;
    _pntSel.ry() += _scrollBar->value();

    if (_columnSelectionMode && !_lineSelectionMode && !_wordSelectionMode) {
        _screenWindow->setSelectionEnd(here.x(), here.y());
    } else {
        _screenWindow->setSelectionEnd(here.x() + offset, here.y());
    }
}

void TerminalDisplay::mouseReleaseEvent(QMouseEvent *ev)
{
    if (!_screenWindow)
        return;

    auto [charColumn, charLine] = getCharacterPosition(ev->pos());

    if (ev->button() == Qt::LeftButton) {
        Q_EMIT isBusySelecting(false);
        if (dragInfo.state == diPending) {
            // We had a drag event pending but never confirmed.  Kill selection
            _screenWindow->clearSelection();
            // emit clearSelectionSignal();
        } else {
            if (_actSel > 1) {
                setSelection(_screenWindow->selectedText(_preserveLineBreaks));
            }

            _actSel = 0;

            // FIXME: emits a release event even if the mouse is
            //        outside the range. The procedure used in `mouseMoveEvent'
            //        applies here, too.

            if (!_mouseMarks && !(ev->modifiers() & Qt::ShiftModifier))
                Q_EMIT mouseSignal(0, charColumn + 1, charLine + 1 + _scrollBar->value() - _scrollBar->maximum(), 2);
        }
        dragInfo.state = diNone;
    }

    if (!_mouseMarks && ((ev->button() == Qt::RightButton && !(ev->modifiers() & Qt::ShiftModifier)) || ev->button() == Qt::MiddleButton)) {
        Q_EMIT mouseSignal(ev->button() == Qt::MiddleButton ? 1 : 2, charColumn + 1, charLine + 1 + _scrollBar->value() - _scrollBar->maximum(), 2);
    }
}

CharPos TerminalDisplay::getCharacterPosition(const QPointF &widgetPoint) const
{
    int line, column = 0;

    line = (widgetPoint.y() - contentsRect().top() - _topMargin) / qRound(_fontHeight);
    if (line < 0)
        line = 0;
    if (line >= _usedLines)
        line = _usedLines - 1;

    int x = widgetPoint.x() + qRound(_fontWidth) / 2 - contentsRect().left() - _leftMargin;
    if (_fixedFont)
        column = x / qRound(_fontWidth);
    else {
        column = 0;
        while (column + 1 < _usedColumns && x > textWidth(0, column + 1, line))
            column++;
    }

    if (column < 0)
        column = 0;

    // the column value returned can be equal to _usedColumns, which
    // is the position just after the last character displayed in a line.
    //
    // this is required so that the user can select characters in the right-most
    // column (or left-most for right-to-left input)
    if (column > _usedColumns)
        column = _usedColumns;

    return {column, line};
}

void TerminalDisplay::updateFilters()
{
    if (!_screenWindow)
        return;

    processFilters();
}

void TerminalDisplay::updateLineProperties()
{
    if (!_screenWindow)
        return;

    _lineProperties = _screenWindow->getLineProperties();
}

void TerminalDisplay::mouseDoubleClickEvent(QMouseEvent *ev)
{
    if (ev->button() != Qt::LeftButton)
        return;
    if (!_screenWindow)
        return;

    QPoint pos = getCharacterPosition(ev->pos());

    // pass on double click as two clicks.
    if (!_mouseMarks && !(ev->modifiers() & Qt::ShiftModifier)) {
        // Send just _ONE_ click event, since the first click of the double click
        // was already sent by the click handler
        Q_EMIT mouseSignal(0, pos.x() + 1, pos.y() + 1 + _scrollBar->value() - _scrollBar->maximum(),
                           0); // left button
        return;
    }

    _screenWindow->clearSelection();
    QPoint bgnSel = pos;
    QPoint endSel = pos;
    int i = loc(bgnSel.x(), bgnSel.y());
    _iPntSel = bgnSel;
    _iPntSel.ry() += _scrollBar->value();

    _wordSelectionMode = true;

    // find word boundaries...
    QChar selClass = charClass(_image[i].character);
    {
        // find the start of the word
        int x = bgnSel.x();
        while (((x > 0) || (bgnSel.y() > 0 && (_lineProperties[bgnSel.y() - 1] & LINE_WRAPPED))) && charClass(_image[i - 1].character) == selClass) {
            i--;
            if (x > 0)
                x--;
            else {
                x = _usedColumns - 1;
                bgnSel.ry()--;
            }
        }

        bgnSel.setX(x);
        _screenWindow->setSelectionStart(bgnSel.x(), bgnSel.y(), false);

        // find the end of the word
        i = loc(endSel.x(), endSel.y());
        x = endSel.x();
        while (((x < _usedColumns - 1) || (endSel.y() < _usedLines - 1 && (_lineProperties[endSel.y()] & LINE_WRAPPED)))
               && charClass(_image[i + 1].character) == selClass) {
            i++;
            if (x < _usedColumns - 1)
                x++;
            else {
                x = 0;
                endSel.ry()++;
            }
        }

        endSel.setX(x);

        // In word selection mode don't select @ (64) if at end of word.
        if ((QChar(_image[i].character) == QLatin1Char('@')) && ((endSel.x() - bgnSel.x()) > 0))
            endSel.setX(x - 1);

        _actSel = 2; // within selection

        _screenWindow->setSelectionEnd(endSel.x(), endSel.y());

        setSelection(_screenWindow->selectedText(_preserveLineBreaks));
    }

    _possibleTripleClick = true;

    QTimer::singleShot(QApplication::doubleClickInterval(), this, SLOT(tripleClickTimeout()));
}

void TerminalDisplay::wheelEvent(QWheelEvent *ev)
{
    if (ev->angleDelta().y() == 0) // orientation is not verical
        return;

    // if the terminal program is not interested mouse events
    // then send the event to the scrollbar if the slider has room to move
    // or otherwise send simulated up / down key presses to the terminal program
    // for the benefit of programs such as 'less'
    if (_mouseMarks) {
        bool canScroll = _scrollBar->maximum() > 0;
        if (canScroll)
            _scrollBar->event(ev);
        else {
            // assume that each Up / Down key event will cause the terminal application
            // to scroll by one line.
            //
            // to get a reasonable scrolling speed, scroll by one line for every 5 degrees
            // of mouse wheel rotation.  Mouse wheels typically move in steps of 15 degrees,
            // giving a scroll of 3 lines
            int key = ev->angleDelta().y() > 0 ? Qt::Key_Up : Qt::Key_Down;

            // QWheelEvent::delta() gives rotation in eighths of a degree
            int wheelDegrees = ev->angleDelta().y() / 8;
            int linesToScroll = abs(wheelDegrees) / 5;

            QKeyEvent keyScrollEvent(QEvent::KeyPress, key, Qt::NoModifier);

            for (int i = 0; i < linesToScroll; i++)
                Q_EMIT keyPressedSignal(&keyScrollEvent, false);
        }
    } else {
        // terminal program wants notification of mouse activity
        auto pos = getCharacterPosition(ev->position());

        Q_EMIT mouseSignal(ev->angleDelta().y() > 0 ? 4 : 5, pos.columns + 1, pos.lines + 1 + _scrollBar->value() - _scrollBar->maximum(), 0);
    }
}

void TerminalDisplay::tripleClickTimeout()
{
    _possibleTripleClick = false;
}

void TerminalDisplay::mouseTripleClickEvent(QMouseEvent *ev)
{
    if (!_screenWindow)
        return;

    _iPntSel = getCharacterPosition(ev->pos());

    _screenWindow->clearSelection();

    _lineSelectionMode = true;
    _wordSelectionMode = false;

    _actSel = 2; // within selection
    Q_EMIT isBusySelecting(true); // Keep it steady...

    while (_iPntSel.y() > 0 && (_lineProperties[_iPntSel.y() - 1] & LINE_WRAPPED))
        _iPntSel.ry()--;

    if (_tripleClickMode == SelectForwardsFromCursor) {
        // find word boundary start
        int i = loc(_iPntSel.x(), _iPntSel.y());
        QChar selClass = charClass(_image[i].character);
        int x = _iPntSel.x();

        while (((x > 0) || (_iPntSel.y() > 0 && (_lineProperties[_iPntSel.y() - 1] & LINE_WRAPPED))) && charClass(_image[i - 1].character) == selClass) {
            i--;
            if (x > 0)
                x--;
            else {
                x = _columns - 1;
                _iPntSel.ry()--;
            }
        }

        _screenWindow->setSelectionStart(x, _iPntSel.y(), false);
        _tripleSelBegin = QPoint(x, _iPntSel.y());
    } else if (_tripleClickMode == SelectWholeLine) {
        _screenWindow->setSelectionStart(0, _iPntSel.y(), false);
        _tripleSelBegin = QPoint(0, _iPntSel.y());
    }

    while (_iPntSel.y() < _lines - 1 && (_lineProperties[_iPntSel.y()] & LINE_WRAPPED))
        _iPntSel.ry()++;

    _screenWindow->setSelectionEnd(_columns - 1, _iPntSel.y());

    setSelection(_screenWindow->selectedText(_preserveLineBreaks));

    _iPntSel.ry() += _scrollBar->value();
}

bool TerminalDisplay::focusNextPrevChild(bool next)
{
    if (next)
        return false; // This disables changing the active part in konqueror
                      // when pressing Tab
    return false;
    // return QWidget::focusNextPrevChild( next );
}

QChar TerminalDisplay::charClass(QChar qch) const
{
    if (qch.isSpace())
        return QLatin1Char(' ');

    if (qch.isLetterOrNumber() || _wordCharacters.contains(qch, Qt::CaseInsensitive))
        return QLatin1Char('a');

    return qch;
}

void TerminalDisplay::setWordCharacters(const QString &wc)
{
    _wordCharacters = wc;
}

void TerminalDisplay::setUsesMouse(bool on)
{
    if (_mouseMarks != on) {
        _mouseMarks = on;
        setCursor(_mouseMarks ? Qt::IBeamCursor : Qt::ArrowCursor);
        Q_EMIT usesMouseChanged();
    }
}
bool TerminalDisplay::usesMouse() const
{
    return _mouseMarks;
}

void TerminalDisplay::setBracketedPasteMode(bool on)
{
    _bracketedPasteMode = on;
}
bool TerminalDisplay::bracketedPasteMode() const
{
    return _bracketedPasteMode;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                               Clipboard                                   */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#undef KeyPress

void TerminalDisplay::emitSelection(bool useXselection, bool appendReturn)
{
    if (!_screenWindow)
        return;

    // Paste Clipboard by simulating keypress events
    QString text = QApplication::clipboard()->text(useXselection ? QClipboard::Selection : QClipboard::Clipboard);
    if (!text.isEmpty()) {
        text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
        text.replace(QLatin1Char('\n'), QLatin1Char('\r'));

        if (_trimPastedTrailingNewlines) {
            text.replace(QRegularExpression(QStringLiteral("\\r+$")), QString());
        }

        // TODO QMLTERMWIDGET We should expose this as a signal.
        //    if (_confirmMultilinePaste && text.contains(QLatin1Char('\r'))) {
        //        QMessageBox confirmation(this);
        //        confirmation.setWindowTitle(tr("Paste multiline text"));
        //        confirmation.setText(tr("Are you sure you want to paste this text?"));
        //        confirmation.setDetailedText(text);
        //        confirmation.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        //        // Click "Show details..." to show those by default
        //        const auto buttons = confirmation.buttons();
        //        for( QAbstractButton * btn : buttons ) {
        //            if (confirmation.buttonRole(btn) == QMessageBox::ActionRole && btn->text() == QMessageBox::tr("Show Details...")) {
        //                Q_EMIT btn->clicked();
        //                break;
        //            }
        //        }
        //        confirmation.setDefaultButton(QMessageBox::Yes);
        //        confirmation.exec();
        //        if (confirmation.standardButton(confirmation.clickedButton()) != QMessageBox::Yes) {
        //            return;
        //        }
        //    }

        bracketText(text);

        // appendReturn is intentionally handled _after_ enclosing texts with brackets as
        // that feature is used to allow execution of commands immediately after paste.
        // Ref: https://bugs.kde.org/show_bug.cgi?id=16179
        // Ref: https://github.com/KDE/konsole/commit/83d365f2ebfe2e659c1e857a2f5f247c556ab571
        if (appendReturn) {
            text.append(QLatin1Char('\r'));
        }

        QKeyEvent e(QEvent::KeyPress, 0, Qt::NoModifier, text);
        Q_EMIT keyPressedSignal(&e, true); // expose as a big fat keypress event

        _screenWindow->clearSelection();

        switch (mMotionAfterPasting) {
        case MoveStartScreenWindow:
            // Temporarily stop tracking output, or pasting contents triggers
            // ScreenWindow::notifyOutputChanged() and the latter scrolls the
            // terminal to the last line. It will be re-enabled when needed
            // (e.g., scrolling to the last line).
            _screenWindow->setTrackOutput(false);
            _screenWindow->scrollTo(0);
            break;
        case MoveEndScreenWindow:
            scrollToEnd();
            break;
        case NoMoveScreenWindow:
            break;
        }
    }
}

void TerminalDisplay::bracketText(QString &text) const
{
    if (bracketedPasteMode() && !_disabledBracketedPasteMode) {
        text.prepend(QLatin1String("\033[200~"));
        text.append(QLatin1String("\033[201~"));
    }
}

void TerminalDisplay::setSelection(const QString &t)
{
    if (QApplication::clipboard()->supportsSelection()) {
        QApplication::clipboard()->setText(t, QClipboard::Selection);
    }
}

void TerminalDisplay::copyClipboard()
{
    if (!_screenWindow)
        return;

    QString text = _screenWindow->selectedText(_preserveLineBreaks);
    if (!text.isEmpty())
        QApplication::clipboard()->setText(text);
}

void TerminalDisplay::pasteClipboard()
{
    emitSelection(false, false);
}

void TerminalDisplay::pasteSelection()
{
    emitSelection(true, false);
}

void TerminalDisplay::setConfirmMultilinePaste(bool confirmMultilinePaste)
{
    _confirmMultilinePaste = confirmMultilinePaste;
}

void TerminalDisplay::setTrimPastedTrailingNewlines(bool trimPastedTrailingNewlines)
{
    _trimPastedTrailingNewlines = trimPastedTrailingNewlines;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                Keyboard                                   */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void TerminalDisplay::setFlowControlWarningEnabled(bool enable)
{
    _flowControlWarningEnabled = enable;

    // if the dialog is currently visible and the flow control warning has
    // been disabled then hide the dialog
    if (!enable)
        outputSuspended(false);
}

void TerminalDisplay::setMotionAfterPasting(MotionAfterPasting action)
{
    mMotionAfterPasting = action;
}

int TerminalDisplay::motionAfterPasting()
{
    return mMotionAfterPasting;
}

void TerminalDisplay::keyPressEvent(QKeyEvent *event)
{
    _actSel = 0; // Key stroke implies a screen update, so TerminalDisplay won't
                 // know where the current selection is.

    if (_hasBlinkingCursor) {
        _blinkCursorTimer->start(QApplication::cursorFlashTime() / 2);
        if (_cursorBlinking)
            blinkCursorEvent();
        else
            _cursorBlinking = false;
    }

    Q_EMIT keyPressedSignal(event, false);

    event->accept();
}

void TerminalDisplay::inputMethodEvent(QInputMethodEvent *event)
{
    QKeyEvent keyEvent(QEvent::KeyPress, 0, Qt::NoModifier, event->commitString());
    Q_EMIT keyPressedSignal(&keyEvent, false);

    _inputMethodData.preeditString = event->preeditString();
    update(preeditRect() | _inputMethodData.previousPreeditRect);

    event->accept();
}

void TerminalDisplay::inputMethodQuery(QInputMethodQueryEvent *event)
{
    event->setValue(Qt::ImEnabled, true);
    event->setValue(Qt::ImHints, QVariant(Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase));
    event->accept();
}

QVariant TerminalDisplay::inputMethodQuery(Qt::InputMethodQuery query) const
{
    const QPoint cursorPos = _screenWindow ? _screenWindow->cursorPosition() : QPoint(0, 0);
    switch (query) {
    case Qt::ImCursorRectangle:
        return imageToWidget(QRect(cursorPos.x(), cursorPos.y(), 1, 1));
        break;
    case Qt::ImFont:
        return font();
        break;
    case Qt::ImCursorPosition:
        // return the cursor position within the current line
        return cursorPos.x();
        break;
    case Qt::ImSurroundingText: {
        // return the text from the current line
        QString lineText;
        QTextStream stream(&lineText);
        PlainTextDecoder decoder;
        decoder.begin(&stream);
        decoder.decodeLine(std::span(&_image[loc(0, cursorPos.y())], _usedColumns), _lineProperties[cursorPos.y()]);
        decoder.end();
        return lineText;
    } break;
    case Qt::ImCurrentSelection:
        return QString();
        break;
    default:
        break;
    }

    return QVariant();
}

bool TerminalDisplay::handleShortcutOverrideEvent(QKeyEvent *keyEvent)
{
    int modifiers = keyEvent->modifiers();

    //  When a possible shortcut combination is pressed,
    //  Q_EMIT the overrideShortcutCheck() signal to allow the host
    //  to decide whether the terminal should override it or not.
    if (modifiers != Qt::NoModifier) {
        int modifierCount = 0;
        unsigned int currentModifier = Qt::ShiftModifier;

        while (currentModifier <= Qt::KeypadModifier) {
            if (modifiers & currentModifier)
                modifierCount++;
            currentModifier <<= 1;
        }
        if (modifierCount < 2) {
            bool override = false;
            Q_EMIT overrideShortcutCheck(keyEvent, override);
            if (override) {
                keyEvent->accept();
                return true;
            }
        }
    }

    // Override any of the following shortcuts because
    // they are needed by the terminal
    int keyCode = keyEvent->key() | modifiers;
    switch (keyCode) {
    // list is taken from the QLineEdit::event() code
    case Qt::Key_Tab:
    case Qt::Key_Delete:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_Backspace:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Escape:
        keyEvent->accept();
        return true;
    }
    return false;
}

bool TerminalDisplay::event(QEvent *event)
{
    bool eventHandled = false;
    switch (event->type()) {
    case QEvent::ShortcutOverride:
        eventHandled = handleShortcutOverrideEvent((QKeyEvent *)event);
        break;
    case QEvent::PaletteChange:
    case QEvent::ApplicationPaletteChange:
        _scrollBar->setPalette(QApplication::palette());
        break;
    case QEvent::InputMethodQuery:
        inputMethodQuery(static_cast<QInputMethodQueryEvent *>(event));
        eventHandled = true;
        break;
    default:
        break;
    }
    return eventHandled ? true : QQuickItem::event(event);
}

void TerminalDisplay::setBellMode(int mode)
{
    _bellMode = mode;
}

void TerminalDisplay::enableBell()
{
    _allowBell = true;
}

void TerminalDisplay::bell(const QString &message)
{
    if (_bellMode == NoBell)
        return;

    // limit the rate at which bells can occur
    //...mainly for sound effects where rapid bells in sequence
    // produce a horrible noise
    if (_allowBell) {
        _allowBell = false;
        QTimer::singleShot(500, this, SLOT(enableBell()));

        if (_bellMode == SystemBeepBell) {
            QApplication::beep();
        } else if (_bellMode == NotifyBell) {
            Q_EMIT notifyBell(message);
        } else if (_bellMode == VisualBell) {
            swapColorTable();
            QTimer::singleShot(200, this, SLOT(swapColorTable()));
        }
    }
}

void TerminalDisplay::selectionChanged()
{
    Q_EMIT copyAvailable(_screenWindow->selectedText(false).isEmpty() == false);
}

void TerminalDisplay::swapColorTable()
{
    ColorEntry color = _colorTable[1];
    _colorTable[1] = _colorTable[0];
    _colorTable[0] = color;
    _colorsInverted = !_colorsInverted;
    update();
}

void TerminalDisplay::clearImage()
{
    // We initialize _image[_imageSize] too. See makeImage()
    for (int i = 0; i <= _imageSize; i++) {
        _image[i].character = u' ';
        _image[i].foregroundColor = CharacterColor(COLOR_SPACE_DEFAULT, DEFAULT_FORE_COLOR);
        _image[i].backgroundColor = CharacterColor(COLOR_SPACE_DEFAULT, DEFAULT_BACK_COLOR);
        _image[i].rendition = DEFAULT_RENDITION;
    }
}

void TerminalDisplay::calcGeometry()
{
    _scrollBar->resize(_scrollBar->sizeHint().width(), contentsRect().height());
    int scrollBarWidth = _scrollBar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, _scrollBar) ? 0 : _scrollBar->width();
    switch (_scrollbarLocation) {
    case QTermWidget::NoScrollBar:
        _leftMargin = _leftBaseMargin;
        _contentWidth = contentsRect().width() - 2 * _leftBaseMargin;
        break;
    case QTermWidget::ScrollBarLeft:
        _leftMargin = _leftBaseMargin + scrollBarWidth;
        _contentWidth = contentsRect().width() - 2 * _leftBaseMargin - scrollBarWidth;
        _scrollBar->move(contentsRect().topLeft());
        break;
    case QTermWidget::ScrollBarRight:
        _leftMargin = _leftBaseMargin;
        _contentWidth = contentsRect().width() - 2 * _leftBaseMargin - scrollBarWidth;
        _scrollBar->move(contentsRect().topRight() - QPoint(_scrollBar->width() - 1, 0));
        break;
    }

    _topMargin = _topBaseMargin;
    _contentHeight = contentsRect().height() - 2 * _topBaseMargin + /* mysterious */ 1;

    if (!_isFixedSize) {
        // ensure that display is always at least one column wide
        _columns = qMax(1, qRound(_contentWidth / _fontWidth));
        _usedColumns = qMin(_usedColumns, _columns);

        // ensure that display is always at least one line high
        _lines = qMax(1, _contentHeight / qRound(_fontHeight));
        _usedLines = qMin(_usedLines, _lines);
    }
}

void TerminalDisplay::makeImage()
{
    calcGeometry();

    // confirm that array will be of non-zero size, since the painting code
    // assumes a non-zero array length
    Q_ASSERT(_lines > 0 && _columns > 0);
    Q_ASSERT(_usedLines <= _lines && _usedColumns <= _columns);

    _imageSize = _lines * _columns;

    // We over-commit one character so that we can be more relaxed in dealing with
    // certain boundary conditions: _image[_imageSize] is a valid but unused position
    _image.resize(_imageSize + 1);

    clearImage();
}

// calculate the needed size, this must be synced with calcGeometry()
void TerminalDisplay::setSize(int columns, int lines)
{
    int scrollBarWidth =
        (_scrollBar->isHidden() || _scrollBar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, _scrollBar)) ? 0 : _scrollBar->sizeHint().width();
    int horizontalMargin = 2 * _leftBaseMargin;
    int verticalMargin = 2 * _topBaseMargin;

    QSize newSize = QSize(horizontalMargin + scrollBarWidth + (columns * _fontWidth), verticalMargin + (lines * qRound(_fontHeight)));

    if (newSize != size()) {
        _size = newSize;
        // updateGeometry();
        // TODO Manage geometry change
    }
}

void TerminalDisplay::setFixedSize(int cols, int lins)
{
    _isFixedSize = true;

    // ensure that display is at least one line by one column in size
    _columns = qMax(1, cols);
    _lines = qMax(1, lins);
    _usedColumns = qMin(_usedColumns, _columns);
    _usedLines = qMin(_usedLines, _lines);

    if (!_image.empty()) {
        _image.clear();
        makeImage();
    }
    setSize(cols, lins);
    // QWidget::setFixedSize(_size);
}

QSize TerminalDisplay::sizeHint() const
{
    return _size;
}

/* --------------------------------------------------------------------- */
/*                                                                       */
/* Drag & Drop                                                           */
/*                                                                       */
/* --------------------------------------------------------------------- */

void TerminalDisplay::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(QLatin1String("text/plain")))
        event->acceptProposedAction();
    if (event->mimeData()->urls().size())
        event->acceptProposedAction();
}

void TerminalDisplay::dropEvent(QDropEvent *event)
{
    // KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    QList<QUrl> urls = event->mimeData()->urls();

    QString dropText;
    if (!urls.isEmpty()) {
        // TODO/FIXME: escape or quote pasted things if neccessary...
        qDebug() << "TerminalDisplay: handling urls. It can be broken. Report any errors, please";
        for (int i = 0; i < urls.size(); i++) {
            // KUrl url = KIO::NetAccess::mostLocalUrl( urls[i] , 0 );
            QUrl url = urls[i];

            QString urlText;

            if (url.isLocalFile())
                urlText = url.path();
            else
                urlText = url.toString();

            // in future it may be useful to be able to insert file names with drag-and-drop
            // without quoting them (this only affects paths with spaces in)
            // urlText = KShell::quoteArg(urlText);

            dropText += urlText;

            if (i != urls.size() - 1)
                dropText += QLatin1Char(' ');
        }
    } else {
        dropText = event->mimeData()->text();
    }

    Q_EMIT sendStringToEmu(dropText.toLocal8Bit().constData());
}

void TerminalDisplay::doDrag()
{
    dragInfo.state = diDragging;
    dragInfo.dragObject = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setText(QApplication::clipboard()->text(QClipboard::Selection));
    dragInfo.dragObject->setMimeData(mimeData);
    dragInfo.dragObject->exec(Qt::CopyAction);
    // Don't delete the QTextDrag object.  Qt will delete it when it's done with it.
}

void TerminalDisplay::outputSuspended(bool suspended)
{
    _outputSuspendedLabel->setVisible(suspended);
}

uint TerminalDisplay::lineSpacing() const
{
    return _lineSpacing;
}

void TerminalDisplay::setLineSpacing(uint i)
{
    if (i != _lineSpacing) {
        _lineSpacing = i;
        setVTFont(font()); // Trigger an update.
        Q_EMIT lineSpacingChanged();
    }
}

int TerminalDisplay::margin() const
{
    return _topBaseMargin;
}

void TerminalDisplay::setMargin(int i)
{
    _topBaseMargin = i;
    _leftBaseMargin = i;
}

// QMLTermWidget specific functions ///////////////////////////////////////////

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void TerminalDisplay::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void TerminalDisplay::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
    if (newGeometry != oldGeometry) {
        resizeEvent(nullptr);
        update();
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
#else
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
#endif
}

void TerminalDisplay::update(const QRegion &region)
{
    Q_UNUSED(region);
    // TODO this function might be optimized
    //        const rects = region.rects();
    //        for (QRect rect : rects) {
    //            QQuickPaintedItem::update(rect.adjusted(-1, -1, +1, +1));
    //        }
    QQuickPaintedItem::update(region.boundingRect().adjusted(-1, -1, +1, +1));

    Q_EMIT imagePainted();
}

void TerminalDisplay::update()
{
    QQuickPaintedItem::update(contentsRect());
}

QRect TerminalDisplay::contentsRect() const
{
    return QRect(0, 0, this->width(), this->height());
}

QSize TerminalDisplay::size() const
{
    return QSize(this->width(), this->height());
}

void TerminalDisplay::setSession(TerminalSession *session)
{
    if (m_session != session) {
        //        qDebug() << "SetSession called";
        //        if (m_session)
        //            m_session->removeView(this);

        m_session = session;

        connect(this, &TerminalDisplay::copyAvailable, m_session, &TerminalSession::selectionChanged);
        connect(this, &TerminalDisplay::termGetFocus, m_session, &TerminalSession::termGetFocus);
        connect(this, &TerminalDisplay::termLostFocus, m_session, &TerminalSession::termLostFocus);
        connect(this, &TerminalDisplay::keyPressedSignal, m_session, &TerminalSession::termKeyPressed);

        m_session->addView(this);

        setRandomSeed(m_session->getRandomSeed());
        update();
        Q_EMIT sessionChanged();
    }
}

TerminalSession *TerminalDisplay::getSession()
{
    return m_session;
}

QStringList TerminalDisplay::availableColorSchemes()
{
    QStringList ret;
    const auto colorSchemes = ColorSchemeManager::instance()->allColorSchemes();
    std::transform(colorSchemes.begin(), colorSchemes.end(), std::back_inserter(ret), [](auto cs) {
        return cs->name();
    });
    return ret;
}

void TerminalDisplay::setColorScheme(const QString &name)
{
    if (name != _colorScheme) {
        const ColorScheme *cs = [&, this]() {
            // avoid legacy (int) solution
            if (!availableColorSchemes().contains(name))
                return ColorSchemeManager::instance()->defaultColorScheme();
            else
                return ColorSchemeManager::instance()->findColorScheme(name);
        }();

        if (!cs) {
            qDebug() << "Cannot load color scheme: " << name;
            return;
        }

        setColorTable(cs->getColorTable());

        setFillColor(cs->backgroundColor());
        _colorScheme = name;
        Q_EMIT colorSchemeChanged();
    }
}

QString TerminalDisplay::colorScheme() const
{
    return _colorScheme;
}

void TerminalDisplay::simulateKeyPress(int key, int modifiers, bool pressed, quint32 nativeScanCode, const QString &text)
{
    Q_UNUSED(nativeScanCode);
    QEvent::Type type = pressed ? QEvent::KeyPress : QEvent::KeyRelease;
    QKeyEvent event = QKeyEvent(type, key, (Qt::KeyboardModifier)modifiers, text);
    keyPressedSignal(&event, false);
}

void TerminalDisplay::simulateKeySequence(const QKeySequence &keySequence)
{
    for (int i = 0; i < keySequence.count(); ++i) {
        const Qt::Key key = Qt::Key(keySequence[i].key());
        const Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(keySequence[i].keyboardModifiers());
        QKeyEvent eventPress = QKeyEvent(QEvent::KeyPress, key, modifiers, QString());
        keyPressedSignal(&eventPress, false);
    }
}

void TerminalDisplay::simulateWheel(int x, int y, int buttons, int modifiers, QPoint angleDelta)
{
    QPoint pixelDelta; // pixelDelta is optional and can be null.
    QWheelEvent event(QPointF(x, y),
                      mapToGlobal(QPointF(x, y)),
                      pixelDelta,
                      angleDelta,
                      (Qt::MouseButtons)buttons,
                      (Qt::KeyboardModifiers)modifiers,
                      Qt::ScrollBegin,
                      false);
    wheelEvent(&event);
}

void TerminalDisplay::simulateMouseMove(int x, int y, int button, int buttons, int modifiers)
{
    QMouseEvent event(QEvent::MouseMove, QPointF(x, y), (Qt::MouseButton)button, (Qt::MouseButtons)buttons, (Qt::KeyboardModifiers)modifiers);
    mouseMoveEvent(&event);
}

void TerminalDisplay::simulateMousePress(int x, int y, int button, int buttons, int modifiers)
{
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(x, y), (Qt::MouseButton)button, (Qt::MouseButtons)buttons, (Qt::KeyboardModifiers)modifiers);
    mousePressEvent(&event);
}

void TerminalDisplay::simulateMouseRelease(int x, int y, int button, int buttons, int modifiers)
{
    QMouseEvent event(QEvent::MouseButtonRelease, QPointF(x, y), (Qt::MouseButton)button, (Qt::MouseButtons)buttons, (Qt::KeyboardModifiers)modifiers);
    mouseReleaseEvent(&event);
}

void TerminalDisplay::simulateMouseDoubleClick(int x, int y, int button, int buttons, int modifiers)
{
    QMouseEvent event(QEvent::MouseButtonDblClick, QPointF(x, y), (Qt::MouseButton)button, (Qt::MouseButtons)buttons, (Qt::KeyboardModifiers)modifiers);
    mouseDoubleClickEvent(&event);
}

QSize TerminalDisplay::getTerminalSize()
{
    return QSize(_lines, _columns);
}

bool TerminalDisplay::getUsesMouse()
{
    return !usesMouse();
}

int TerminalDisplay::getScrollbarValue()
{
    return _scrollBar->value();
}

int TerminalDisplay::getScrollbarMaximum()
{
    return _scrollBar->maximum();
}

int TerminalDisplay::getScrollbarMinimum()
{
    return _scrollBar->minimum();
}

QSize TerminalDisplay::getFontMetrics()
{
    return QSize(qRound(_fontWidth), qRound(_fontHeight));
}

void TerminalDisplay::setFullCursorHeight(bool val)
{
    if (m_full_cursor_height != val) {
        m_full_cursor_height = val;
        Q_EMIT fullCursorHeightChanged();
    }
}

bool TerminalDisplay::fullCursorHeight() const
{
    return m_full_cursor_height;
}

void TerminalDisplay::itemChange(ItemChange change, const ItemChangeData &value)
{
    switch (change) {
    case QQuickItem::ItemVisibleHasChanged:
        if (value.boolValue && _screenWindow) {
            if (this->columns() != _screenWindow->columnCount() || this->lines() != _screenWindow->lineCount()) {
                Q_EMIT changedContentSizeSignal(_contentHeight, _contentWidth);
            }
        }
        break;
    default:
        break;
    }

    QQuickPaintedItem::itemChange(change, value);
}
