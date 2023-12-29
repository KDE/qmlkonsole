/*  SPDX-FileCopyrightText: 2008 e _k (e_k@users.sourceforge.net)

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _Q_TERM_WIDGET
#define _Q_TERM_WIDGET

#include "Emulation.h"
#include "Filter.h"
#include <QTranslator>
#include <QWidget>

class QVBoxLayout;
class TermWidgetImpl;
class SearchBar;
class QUrl;

class QTermWidget
{
public:
    /**
     * This enum describes the location where the scroll bar is positioned in the display widget.
     */
    enum ScrollBarPosition {
        /** Do not show the scroll bar. */
        NoScrollBar = 0,
        /** Show the scroll bar on the left side of the display. */
        ScrollBarLeft = 1,
        /** Show the scroll bar on the right side of the display. */
        ScrollBarRight = 2
    };

    using KeyboardCursorShape = Konsole::Emulation::KeyboardCursorShape;
};

#endif
