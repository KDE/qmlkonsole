/*
    SPDX-FileCopyrightText: 2013 Christian Surlykke

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
#ifndef TASK_H
#define TASK_H

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QRegExp>

#include <ScreenWindow.h>
#include <Session.h>

#include "Emulation.h"
#include "TerminalCharacterDecoder.h"

class QRegExp;

using namespace Konsole;

typedef QPointer<Emulation> EmulationPtr;

class HistorySearch : public QObject
{
    Q_OBJECT

public:
    explicit HistorySearch(EmulationPtr emulation, const QRegExp &regExp, bool forwards, int startColumn, int startLine, QObject *parent);

    ~HistorySearch() override;

    void search();

Q_SIGNALS:
    void matchFound(int startColumn, int startLine, int endColumn, int endLine);
    void noMatchFound();

private:
    bool search(int startColumn, int startLine, int endColumn, int endLine);
    int findLineNumberInString(QList<int> linePositions, int position);

    EmulationPtr m_emulation;
    QRegExp m_regExp;
    bool m_forwards;
    int m_startColumn;
    int m_startLine;

    int m_foundStartColumn;
    int m_foundStartLine;
    int m_foundEndColumn;
    int m_foundEndLine;
};

#endif /* TASK_H */
