#include "GameBoard.h"
#include <math.h>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include "Globals.h"
#include "DebugObject.h"

const int SMALL_CELL_SIZE = 40;
const int BIG_CELL_SIZE = 80;
const int BB10_CELL_SIZE = 95;
const int DEFAULT_ROW_COUNT = 8;
const int DEFAULT_COLUMN_COUNT = 8;

/* This is msecs. Half of second is enough for smooth animation. */
const int TIMER_INTERVAL = 500;
const int ENOUGH_TIME_TO_DIE = 1000;

const int ANIMATION_STOP_THRESHOLD = 3;
const int LEVEL_CAP_MULTIPLYER = 60;

const int HYPER_CUBE_MULTIPLYER = 2;
const double DIFFICULTY_MULTIPLYER = 1.07;

GameBoard::GameBoard(QDeclarativeItem *parent): QDeclarativeItem(parent)
{
    m_rowCount = DEFAULT_ROW_COUNT;
    m_columnCount = DEFAULT_COLUMN_COUNT;
    for (int i = 0; i < m_rowCount*m_columnCount; ++i)
        m_boardData.append(NULL);

    /* Will create engine on demand */
    m_engineInited = false;
    m_component = NULL;
    m_textComponent = NULL;

    /* Setting initial values for level and score */
    setLevel(1);
    setScore(0);

    /* Setting scene, delays and timers object */
    m_scene = this->scene();
    m_currentStepDelay = 0;
    m_timer.setInterval(TIMER_INTERVAL);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(checkGemPositions()));

    m_selectedGem = NULL;
    m_gemSelected = false;
    m_selGemRow = 0;
    m_selGemColumn = 0;
    m_gemMovedByUser = false;
    m_userInteractionAccepted = true;
    m_gameStarted = false;
    m_gameLost = false;
    m_cellSize = BB10_CELL_SIZE;
}

GameBoard::~GameBoard() {
    /* Game is saved only when it was started and wasn't lost */
    if (m_gameStarted && (!m_gameLost))
        saveBoardStateToFile();
    clearBoard();
    delete m_component;
    delete m_textComponent;
}


/* Creating engine and components for dynamic object creation */
void GameBoard::initEngine()
{
    if (m_engineInited)
        return;

    if (g_mainEngine != NULL) {
        m_engine = g_mainEngine;
    } else {
        m_engine = new QDeclarativeEngine();
        g_DbgObj() << "[initEngine] Can't use global engine";
        g_DbgObj().flush();
    }

    m_engine->rootContext()->setContextProperty("g_scaleFactor", cellSize()*1.0/40);

    /* Gem cell component */
    m_component = new QDeclarativeComponent(m_engine, QUrl("qrc:/Block.qml"));
    if (!m_component->isReady()) {
        g_DbgObj() << "[GameBoard] Can't fetch gem component from file" << "\r\n";
        g_DbgObj() << "[GameBoard] Ready: " << m_component->isReady() << "\r\n";
        g_DbgObj() << "[GameBoard] Error: " << m_component->isError() << "\r\n";
        g_DbgObj() << "[GameBoard] Loading: " << m_component->isLoading() << "\r\n";
        g_DbgObj() << "[GameBoard] Null: " << m_component->isNull() << "\r\n";
        g_DbgObj() << "[GameBoard] importPathList\r\n";
        g_DbgObj() << g_mainEngine->importPathList().join("\r\n") << "\r\n";
        if (m_component->isError()) {
            QListIterator<QDeclarativeError> i(m_component->errors());
            g_DbgObj() << "[GameBoard] Component errors: " << "\r\n";
            while (i.hasNext()) {
                QDeclarativeError error = i.next();
                g_DbgObj() << "\t" << error.toString() << "\r\n";
            }
        }
        g_DbgObj().flush();
        return;
    }

    /* Score text component */
    m_textComponent = new QDeclarativeComponent(m_engine, QUrl("qrc:/ScoreText.qml"));
    if (!m_textComponent->isReady()) {
        g_DbgObj() << "[GameBoard] Can't fetch score text component from file";
        g_DbgObj().flush();
        return;
    }

    m_engineInited = true;
}

void GameBoard::setCell(int row, int column, GemCell *value)
{
    if (!cellInBoard(row, column))
        return;

    m_boardData[row*m_columnCount + column] = value;
}

/* Resets board for new level. Saves gem modifiers and restores it after new board is created. Also
checks for combos in newly created board and changes gem types so there are no combos */
void GameBoard::resetBoard()
{
    saveGemModifiers();
    clearBoard();

    for (int i = 0; i < m_rowCount; ++i) {
        for (int j = 0; j < m_columnCount; ++j) {
            GemCell *block = createBlock(i, j, i - m_rowCount);
            Q_ASSERT(block != NULL);

            /* Initial variables for combo in cell and combo in row */
            int blockType = block->property("type").toInt();
            bool comboInRow = (safeGetCellType(i, j - 1) == blockType)
                && (safeGetCellType(i, j - 2) == blockType);
            bool comboInColumn = (safeGetCellType(i - 1, j) == blockType)
                && (safeGetCellType(i - 2, j) == blockType);

            /* Generating new types until appropriate found */
            while (comboInRow || comboInColumn) {
                int newType = generateCellType();
                block->setProperty("type", newType);
                comboInRow = (safeGetCellType(i, j - 1) == newType)
                    && (safeGetCellType(i, j - 2) == newType);
                comboInColumn = (safeGetCellType(i - 1, j) == newType)
                    && (safeGetCellType(i - 2, j) == newType);
            }
        }
    }
    restoreGemModifiers();
}

bool GameBoard::markCombos()
{
    /* Special HyperCube processing, finding how many hypercubes are used and type to destroy */
    if (m_gemMovedByUser && hyperCubeUsed()) {
        int typeToDestroy = -1;
        int hyperCubeIndex = -1;
        int hyperCubeIndex2 = -1;

        GemCell *gem1 = m_boardData[m_usrIdx1];
        GemCell *gem2 = m_boardData[m_usrIdx2];

        if (gem1->modifier() != GemCell::HyperCube) {
            typeToDestroy = gem1->property("type").toInt();
            hyperCubeIndex = m_usrIdx2;
        } else if (gem2->modifier() != GemCell::HyperCube) {
            typeToDestroy = gem2->property("type").toInt();
            hyperCubeIndex = m_usrIdx1;
        } else {
            typeToDestroy = generateCellType();
            hyperCubeIndex = m_usrIdx1;
            hyperCubeIndex2 = m_usrIdx2;
        }

        for (int i = 0; i < m_boardData.count(); ++i) {
            bool typeFound =  (m_boardData[i] != NULL)
                &&(m_boardData[i]->property("type") == typeToDestroy);

            if (typeFound) {
                m_boardData[i]->setShouldBeRemoved(true);
                addHyperCubeScoreItem(i / m_columnCount, i % m_columnCount
                    , m_boardData[i]->property("type").toInt());
                if (m_boardData[i]->modifier() != GemCell::Explosive)
                    m_boardData[i]->setExplodedOnce(true);
            }
        }

        m_boardData[hyperCubeIndex]->setShouldBeRemoved(true);
        if (hyperCubeIndex2 != -1)
            m_boardData[hyperCubeIndex2]->setShouldBeRemoved(true);

        return true;
    }


    bool found = false;
    /* Looking in rows */
    for (int row = 0; row < m_rowCount; ++row) {
        bool foundInLine = markCombosInLine(row, GameBoard::Row);
        found = found || foundInLine;
    }
    /* Looking in columns */
    for (int column = 0; column < m_rowCount; ++column) {
        bool foundInLine = markCombosInLine(column, GameBoard::Column);
        found = found || foundInLine;
    }

    return found;
}

/* Funciton marks combos in rows and columnd dependent on which direction parameter is passes. It is
used to gather all checks for combo in one place */
bool GameBoard::markCombosInLine(int lineIndex, Direction direction)
{
    int stopValue;
    if (direction == Row) {
        stopValue = m_columnCount;
    } else {
        stopValue = m_rowCount;
    }

    bool found = false;
    int cnt = 0;
    int lastType = -1;
    for (int i = 0; i < stopValue; ++i) {
        GemCell *cell;

        /* Cell index differs for different directions */
        if (direction == Row) {
            cell = board(lineIndex, i);
        } else {
            cell = board(i, lineIndex);
        };

        int type = -1;
        if (cell != NULL)
            type = cell->property("type").toInt();

        /* To increase count cell must be not NULL and has the same type as previous */
        if ((cell != NULL) && (type == lastType))
            cnt++;

        /* Three conditions to check for combo: NULL cell, type changed, row ended */
        if ((cell == NULL) || (type != lastType) || (i == stopValue - 1)){
            /* Small workaround for end-of-line combo */
            if ((i == stopValue - 1) && (type == lastType))
                ++i;

            /* Checking for combo */
            if (cnt >= 3) {
                /* Adding score item changes for different directions */
                if (direction == Row) {
                    addScoreItem(lineIndex, i, lastType, GameBoard::Row, cnt);
                } else {
                    addScoreItem(i, lineIndex, lastType, GameBoard::Column, cnt);
                }

                found = true;
                while (cnt > 0) {
                    int nextIndex;
                    /* Reverse movement is changed for different directions */
                    if (direction == Row) {
                        nextIndex = index(lineIndex, i - cnt);
                    } else {
                        nextIndex = index(i - cnt, lineIndex);
                    }
                    m_boardData[nextIndex]->setShouldBeRemoved(true);
                    --cnt;
                }
            }

            /* Resetting the counter */
            cnt = 1;
            if (cell != NULL) {
                lastType = type;
            } else {
                lastType = -1;
            }
        }
    }

    return found;
}

bool GameBoard::cellInBoard(int row, int column)
{
    if ((row < 0) || (column < 0) || (row >= m_rowCount) || (column >= m_columnCount)) {
        return false;
    } else {
        return true;
    }
}

int GameBoard::index(int row, int column)
{
    return (row*m_columnCount + column);
}

/* Function to create block. Automatically adds it to the game board. */
GemCell * GameBoard::createBlock(int row, int column, int startRow)
{
    if (!m_engineInited)
        initEngine();

    Q_ASSERT(m_component != NULL);
    Q_ASSERT(scene() != NULL);
    if (!cellInBoard(row, column))
        return NULL;

    GemCell *curCell = qobject_cast<GemCell *>(m_component->create());
    this->scene()->addItem(curCell);
    curCell->setParentItem(this);
    curCell->setProperty("type", generateCellType());
    curCell->setWidth(cellSize());
    curCell->setHeight(cellSize());
    curCell->setY(startRow*cellSize());
    curCell->setProperty("behaviorPause", abs(startRow)*50 + m_currentStepDelay);
    curCell->setProperty("x", column*cellSize());
    curCell->setProperty("y", row*cellSize());
    curCell->setProperty("spawned", true);
    curCell->setProperty("srcSize", cellSize());
    curCell->setModifier(GemCell::Normal);
    m_boardData[index(row, column)] = curCell;
    return curCell;
}


/* Generate random cell type. Utility function to use in different places. */
int GameBoard::generateCellType()
{
    return floor(rand()*7.0/RAND_MAX);
}

/* Clears game board. Gems are simple deleted, so there is no cute dying animation. Also, current
gem must be deselected, because if not -- using it after board is cleared will cause exception. */
void GameBoard::clearBoard()
{
    deselectCurrentGem();
    for (int i = 0; i < m_rowCount*m_columnCount; ++i) {
        delete m_boardData[i];
        m_boardData[i] = NULL;
    }
}

/* Removes combos from board. These are 'simple' gems. Exploded gems are removed by other
function. */
void GameBoard::removeCombos()
{
    QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; i < m_boardData.count(); ++i) {
        if (m_boardData[i] == NULL)
            continue;
        bool needToProcess = m_boardData[i]->shouldBeRemoved()
            && (!m_boardData[i]->invincible());
        if (needToProcess) {
            m_boardData[i]->setTimeToDie(true);
            m_zombieItems.append(qMakePair(now, (QDeclarativeItem *)m_boardData[i]));
            m_boardData[i] = NULL;
        }
    }
}

/* Removes exploded gems from board. Also adds 'explosion effect' bouncing gems from explosion.
Returns true if there were explosions and we should add pause for animation to work. */
bool GameBoard::removeExplosions()
{
    bool thereAreExplosions = false;

    /* If there are explosions -- move upper gems up to create explosions effect */
    /* This code actually won't work at the moment because of main processing loop realisation and
    due to the lack of pauses between different steps */

//    for (int column = 0; column < m_columnCount; ++column) {
//        int expCount = 0;
//        for (int row = m_rowCount - 1; row >= 0; --row) {
//            GemCell *curCell = board(row, column);
//            if (curCell == NULL)
//                continue;

//            bool isExplosion = (curCell->shouldBeRemoved() == true)
//                && (curCell->modifier() == GemCell::Explosive)
//                && (!curCell->invincible());
//            if (isExplosion) {
//                ++expCount;
//                thereAreExplosions = true;
//            } else {
//                if (expCount != 0) {
//                    qDebug() << "[removeExplosions] Moving gem up";
//                    int curY = curCell->property("y").toInt();
//                    int curPause = curCell->property("behaviorPause").toInt();
//                    curCell->setProperty("behaviorPause", curPause + m_currentStepDelay);
//                    curCell->setProperty("y", curY - CELL_SIZE/2*expCount);
//                }
//            }
//        }
//    }

    /* Actually removing explosions */
    QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; i < m_boardData.count(); ++i) {
        if (m_boardData[i] == NULL)
            continue;
        bool needToProcess = (m_boardData[i]->shouldBeRemoved() == true)
            && (m_boardData[i]->explodedOnce() == true);
        if (needToProcess) {
            m_boardData[i]->setTimeToDie(true);
            m_zombieItems.append(qMakePair(now, (QDeclarativeItem *)m_boardData[i]));
            m_boardData[i] = NULL;
        }
    }

    return thereAreExplosions;
}

void GameBoard::shuffleDown()
{
    for (int column = 0; column < m_columnCount; ++column) {
        int fallDist = 0;
        for (int row = m_rowCount - 1; row >= 0; --row) {
            if (board(row, column) == NULL) {
                fallDist += 1;
            } else {
                if (fallDist > 0) {
                   GemCell* obj = board(row, column);
                   int curPause = obj->property("behaviorPause").toInt();
                   obj->setProperty("behaviorPause", curPause + m_currentStepDelay);
                   obj->setProperty("y", (row + fallDist) * cellSize());
                   m_boardData[index(row + fallDist, column)] = obj;
                   m_boardData[index(row, column)] = NULL;
                }
            }
        }
    }
}

void GameBoard::fillBoard()
{
    removeZombies();

    for (int col = 0; col < m_columnCount; col++) {
        int cnt = 1;
        for (int row = m_rowCount - 1; row >= 0; row--) {
            if (board(row, col) == NULL) {
                createBlock(row, col, -cnt);
                cnt++;
            }
        }
    }
}

void GameBoard::removeZombies()
{
    QMutableListIterator<QPair<QDateTime, QDeclarativeItem *> > i(m_zombieItems);
    QDateTime now = QDateTime::currentDateTime();
    while (i.hasNext()) {
        QPair<QDateTime, QDeclarativeItem *> current = i.next();
        if (current.first.msecsTo(now) > ENOUGH_TIME_TO_DIE) {
            if (current.second == m_selectedGem)
                deselectCurrentGem();
            delete current.second;
            i.remove();
        }
    }
}

/* One round of removing gems from board. Using timer to check for animation end. */
void GameBoard::removeAll() {
    bool thereAreCombos = markCombos();
    if (thereAreCombos) {
        markIntersections();
        markBonusGems();
        markExplosions();

        showFloatingScores();
        removeExplosions();
        removeCombos();
        m_currentStepDelay += 100;

        shuffleDown();
        m_currentStepDelay += 100;

        fillBoard();
        resetInvincibleStatus();

        /* We are moving gems now, not user */
        m_gemMovedByUser = false;

        m_timer.start();
    } else {
        /* If there are no combos and this is initiated by user -- roll back */
        if (m_gemMovedByUser) {
            switchGems(m_usrIdx1, m_usrIdx2);
            m_gemMovedByUser = false;
        } else if (!hasPossibleCombos()) {
            checkForHighScore();
            emit noMoreMoves();
            m_gameLost = true;
            QTimer::singleShot(800, this, SLOT(dropGemsDown()));
        } else {
            if (score() >= m_currentLevelCap) {
                setLevel(level() + 1);
                emit levelUp();
            }
        }
        m_userInteractionAccepted = true;
    }

    m_currentStepDelay = 0;
}

void GameBoard::loadTestBoard()
{
    QFile inFile("test.board");
    if (inFile.open(QFile::ReadOnly)) {
        QTextStream inStream(&inFile);
        QString strBoard, strBoardLine;
        do {
            strBoardLine = inStream.readLine();
            strBoard += strBoardLine;
        } while (!strBoardLine.isNull());
        fromString(strBoard);
    } else {
        qDebug() << "[loadTestBoard] Can't open test file";
    }
}

void GameBoard::checkGemPositions()
{
    bool result = true;
//    qDebug("[checkGemPositions] Entered");
    for (int i = 0; i < m_boardData.count(); ++i) {
        if (m_boardData[i] != NULL) {
            int row = i / m_columnCount;
            int column = i % m_columnCount;
            /* Checking for not the exact place but rather near of it. Because of spring animation
            could take a long time to stop */
            bool inPlace = (abs(m_boardData[i]->x() - column*cellSize())
                + abs(m_boardData[i]->y() - row*cellSize()) < 3);
            if (!inPlace) {
                result = false;
                break;
            }
        }
    }
    if (result == false) {
//        qDebug("[checkGemPositions] Some gems aren't in place");
        m_timer.start();
    } else {
//        qDebug("[checkGemPositions] All gems are in place, trying to remove again");
        m_timer.stop();
        removeAll();
    }
}

void GameBoard::dbgPrintGemPositions()
{
    for (int i = 0; i < m_boardData.count(); ++i) {
        if (m_boardData[i] != NULL) {
            int row = i / m_columnCount;
            int column = i % m_columnCount;
            qDebug() << "Row: " << row << "Column: " << column << "x: " << m_boardData[i]->x() << "y: " << m_boardData[i]->y();
        }
    }
}

void GameBoard::setScore(int score)
{
    if (m_score != score) {
        m_score = score;
        emit scoreChanged();
    }
}

int GameBoard::levelFromScore()
{
    return (m_score / 1000 + 1);
}

void GameBoard::handleClick(int x, int y)
{
    int row = y / cellSize();
    int column = x / cellSize();


    if (!cellInBoard(row, column))
        return;

    int idx = row*m_columnCount + column;
    GemCell *clickedCell = m_boardData[idx];

    if (m_selectedGem == NULL) {
        if (clickedCell != NULL)
            selectGem(row, column);
    } else {
        if (pow(row - m_selGemRow, 2) + pow(column - m_selGemColumn, 2) != 1) {
            deselectCurrentGem();
            selectGem(row, column);
            return;
        } else {
            if (m_userInteractionAccepted) {
                /* Removing hint arrow */
                setProperty("hintVisible", false);

                /* Remembering user choice to use later if needed */
                m_usrIdx1 = index(m_selGemRow, m_selGemColumn);
                m_usrIdx2 = index(row, column);

                /* If hyperCube used we shouldn't switch gems */
                if (!hyperCubeUsed())
                    switchGems(m_usrIdx1, m_usrIdx2);

                /* Deselecting current cell to remove selection marker */
                deselectCurrentGem();

                /* Remember that user did this */
                m_gemMovedByUser = true;

                /* Starting to count combos */
                m_comboCount = 0;

                /* Don't accept user actions until finished */
                m_userInteractionAccepted = false;

                /* Fire up timer to wait until moving gem animation ends */
                m_timer.start();
            }
        }
    }
}

void GameBoard::selectGem(int row, int column)
{
    Q_ASSERT(m_boardData[index(row, column)] != NULL);
    m_boardData[index(row, column)]->setProperty("selected", true);
    m_selectedGem = m_boardData[index(row, column)];
    setSelGemRow(row);
    setSelGemColumn(column);
    setGemSelected(true);
}

void GameBoard::deselectCurrentGem()
{
    if (m_selectedGem != NULL) {
        m_selectedGem->setProperty("selected", false);
        m_selectedGem = NULL;
        setGemSelected(false);
    }
}

void GameBoard::markExplosions()
{
    for (int row = 0; row < m_rowCount; ++row) {
        for (int column = 0; column < m_columnCount; ++column) {
            GemCell *curCell = m_boardData[index(row, column)];
            bool needToExplode = (curCell != NULL)
                && (curCell->shouldBeRemoved())
                && ((curCell->modifier() == GemCell::Explosive)
                    || (curCell->modifier() == GemCell::RowColumnRemove));
            if (needToExplode) {
                explodeGem(row, column);
            }
        }
    }
}

void GameBoard::explodeGem(int row, int column)
{
    if (!cellInBoard(row, column))
        return;
    GemCell *cell = m_boardData[index(row, column)];
    if (cell == NULL)
        return;
    if (cell->explodedOnce())
        return;
    if (cell->invincible())
        return;

    cell->setExplodedOnce(true);
    if (!cell->shouldBeRemoved())
        cell->setShouldBeRemoved(true);

    /* Processing special bonus gems */
    if (cell->modifier() == GemCell::Explosive) {
        /* Explosive gem destroys 3x3 square with center in gem */
        explodeGem(row - 1, column - 1);
        explodeGem(row - 1, column - 0);
        explodeGem(row - 1, column + 1);

        explodeGem(row - 0, column - 1);
        /* (row - 0, column - 0) is this gem */
        explodeGem(row - 0, column + 1);

        explodeGem(row + 1, column - 1);
        explodeGem(row + 1, column - 0);
        explodeGem(row + 1, column + 1);
    } else if (cell->modifier() == GemCell::RowColumnRemove) {
        /* Intersection gem destroys whole row and whole column where it is situated */
        for (int i = 0; i < m_columnCount; ++i) {
            if (i != column)
                explodeGem(row, i);
        }

        for (int j = 0; j < m_rowCount; ++j) {
            if (j != row)
                explodeGem(j, column);
        }
    }
}

void GameBoard::setSelGemRow(int row)
{
    if (m_selGemRow != row) {
        m_selGemRow = row;
        emit selGemRowChanged();
    }
}

void GameBoard::setSelGemColumn(int column)
{
    if (m_selGemColumn != column) {
        m_selGemColumn = column;
        emit selGemColumnChanged();
    }
}

void GameBoard::setGemSelected(bool newValue)
{
    if (m_gemSelected != newValue) {
        m_gemSelected = newValue;
        emit gemSelectedChanged();
    }
}

void GameBoard::markIntersections()
{
    for (int row = 0; row < m_rowCount; row++) {
         for (int col = 0; col < m_columnCount; col++) {
             if (board(row, col) == NULL)
                 continue;
             if (!board(row, col)->shouldBeRemoved())
                 continue;

             bool isIntersection = hasRowCombo(row, col) && hasColumnCombo(row, col);
             if (isIntersection) {
                 board(row, col)->setModifier(GemCell::RowColumnRemove);
                 board(row, col)->setInvincible(true);
             }
         }
    }
}


/* Function to mark 4x and 5x bonus gems. Should be called after intersections are marked for not
creating two bonus gems where it should be one */
void GameBoard::markBonusGems()
{
    /* Checking rows */
    int cnt;
    int lastType;
    for (int row = 0; row < m_rowCount; row++) {
        cnt = 0;
        lastType = -1;
        for (int col = 0; col < m_columnCount; col++) {
            int idx = index(row, col);
            GemCell *cell = m_boardData[idx];
            int type = -1;
            if (cell != NULL)
                type = cell->property("type").toInt();

            /* Invincible cells were created earlier this turn. So THEY are real bonus gems and
            we shouldn't add more */
            if ((cell != NULL) && (type == lastType) && (!cell->invincible())) {
                cnt++;
            } else {
                /* Checking for explosive bonus gem */
                if (cnt == 4) {
                    bool userBonusGem = m_gemMovedByUser
                        && ((index(row, col - 3) == m_usrIdx1)
                            || (index(row, col - 3) == m_usrIdx2));

                    int bonusGemIndex;
                    if (userBonusGem) {
                        bonusGemIndex = index(row, col - 3);
                    } else {
                        bonusGemIndex = index(row, col - 2);
                    }

                    m_boardData[bonusGemIndex]->setInvincible(true);
                    m_boardData[bonusGemIndex]->setModifier(GemCell::Explosive);
                }

                /* Checking for hypercube bonus gem */
                if (cnt >= 5) {
                    m_boardData[index(row, col - ceil(cnt*1.0/2))]->setInvincible(true);
                    m_boardData[index(row, col - ceil(cnt*1.0/2))]->setModifier(GemCell::HyperCube);
                }

                /* Counting from 1 again */
                cnt = 1;
                if (cell != NULL) {
                    lastType = type;
                } else {
                    lastType = -1;
                }
            }
        }

        /* Checking for combo at end of row */
        /* Checking for explosive bonus gem */
        if (cnt == 4) {
            bool userBonusGem = m_gemMovedByUser
                && ((index(row, m_columnCount - 3) == m_usrIdx1)
                    || (index(row, m_columnCount - 3) == m_usrIdx2));

            int bonusGemIndex;
            if (userBonusGem) {
                bonusGemIndex = index(row, m_columnCount - 3);
            } else {
                bonusGemIndex = index(row, m_columnCount - 2);
            }

            m_boardData[bonusGemIndex]->setInvincible(true);
            m_boardData[bonusGemIndex]->setModifier(GemCell::Explosive);
        }

        /* Checking for hypercube bonus gem */
        if (cnt == 5) {
            m_boardData[index(row, m_columnCount - ceil(cnt*1.0/2))]->setInvincible(true);
            m_boardData[index(row, m_columnCount - ceil(cnt*1.0/2))]->setModifier(GemCell::HyperCube);
        }
    }

    /* And columns */
    for (int col = 0; col < m_columnCount; col++) {
        cnt = 0;
        lastType = -1;
        for (int row = 0; row < m_rowCount; row++) {
            int idx = index(row, col);
            GemCell *cell = m_boardData[idx];
            int type = -1;
            if (cell != NULL)
                type = cell->property("type").toInt();

            /* Invincible cells were created earlier this turn. So THEY are real bonus gems and
            we shouldn't add more */
            if ((cell != NULL) && (type == lastType) && (!cell->invincible())) {
                cnt++;
            } else {
                /* Checking for explosive bonus gem */
                if (cnt == 4) {
                    bool userBonusGem = m_gemMovedByUser
                        && ((index(row - 3, col) == m_usrIdx1)
                            || (index(row - 3, col) == m_usrIdx2));

                    int bonusGemIndex;
                    if (userBonusGem) {
                        bonusGemIndex = index(row - 3, col);
                    } else {
                        bonusGemIndex = index(row - 2, col);
                    }

                    m_boardData[bonusGemIndex]->setInvincible(true);
                    m_boardData[bonusGemIndex]->setModifier(GemCell::Explosive);
                }

                /* Checking for hypercube bonus gem */
                if (cnt == 5) {
                    m_boardData[index(row - ceil(cnt*1.0/2), col)]->setInvincible(true);
                    m_boardData[index(row - ceil(cnt*1.0/2), col)]->setModifier(GemCell::HyperCube);
                }

                /* Counting from 1 again */
                cnt = 1;
                if (cell != NULL) {
                    lastType = type;
                } else {
                    lastType = -1;
                }
            }
        }

        /* Checking for combo at end of column */
        /* Checking for explosive bonus gem */
        if (cnt == 4) {
            bool userBonusGem = m_gemMovedByUser
                && ((index(m_rowCount - 3, col) == m_usrIdx1)
                    || (index(m_rowCount - 3, col) == m_usrIdx2));

            int bonusGemIndex;
            if (userBonusGem) {
                bonusGemIndex = index(m_rowCount - 3, col);
            } else {
                bonusGemIndex = index(m_rowCount - 2, col);
            }

            m_boardData[bonusGemIndex]->setInvincible(true);
            m_boardData[bonusGemIndex]->setModifier(GemCell::Explosive);
        }

        /* Checking for hypercube bonus gem */
        if (cnt == 5) {
            m_boardData[index(m_rowCount - ceil(cnt*1.0/2), col)]->setInvincible(true);
            m_boardData[index(m_rowCount - ceil(cnt*1.0/2), col)]->setModifier(GemCell::HyperCube);
        }
    }
}

/* Special function that returns NULL if you crossed array dimensions. Useful in some situations. */
GemCell * GameBoard::safeGetCell(int row, int column)
{
    if (!cellInBoard(row, column))
        return NULL;

    return m_boardData[index(row, column)];
}

/* Special function that return -1 for cell gem type if this cell is null or out of board */
int GameBoard::safeGetCellType(int row, int column)
{
    if (!cellInBoard(row, column)) {
        return -1;
    } else if (board(row, column) == NULL) {
        return -1;
    } else {
        return board(row, column)->property("type").toInt();
    }
}

/* Function to remove 'invincible' status from all gems. Called at end of one round of gem removal.
Bonus gems that were created during round are invincible for this round, but can be destructed in
the next one. Also resets some other statuses, yhat should be cleared for next round of removal. */
void GameBoard::resetInvincibleStatus()
{
    for (int i = 0; i < m_boardData.count(); ++i) {
        if (m_boardData[i] != NULL) {
            m_boardData[i]->setInvincible(false);
            m_boardData[i]->setExplodedOnce(false);
            m_boardData[i]->setShouldBeRemoved(false);
        }
    }
}


/* Function used to switch gems by indexes. Can be used to switch gems around if called twice. If
used to switch gems around then need to wait until animation ends some way */
void GameBoard::switchGems(int idx1, int idx2)
{
    int row1 = idx1 / m_columnCount;
    int column1 = idx1 % m_columnCount;
    int row2 = idx2 / m_columnCount;
    int column2 = idx2 % m_columnCount;
    if ((!cellInBoard(row1, column1)) || (!cellInBoard(row2, column2)))
        return;

    GemCell *firstGemCell = m_boardData[idx1];
    GemCell *secondGemCell = m_boardData[idx2];

    if ((firstGemCell == NULL) || (secondGemCell == NULL))
        return;

    /* Switching in game board */
    m_boardData[idx1] = secondGemCell;
    m_boardData[idx2] = firstGemCell;

    /* First gem should always has higher 'z' than second. It enables some nice animation */
    firstGemCell->setZValue(1);
    secondGemCell->setZValue(0);

    /* Should use columns and rows here because we don't want to be affected by coordinates in the
    middle of some animation */
    /* Switching on the screen if necessary */
    if (column1 != column2) {
        firstGemCell->setProperty("x", column2*cellSize());
        secondGemCell->setProperty("x", column1*cellSize());
    }

    /* Switching on the screen if necessary */
    if (row1 != row2) {
        firstGemCell->setProperty("y", row2*cellSize());
        secondGemCell->setProperty("y", row1*cellSize());
    }
}

bool GameBoard::hyperCubeUsed()
{
    Q_ASSERT((m_usrIdx1 >= 0) && (m_usrIdx1 < m_boardData.count()));
    Q_ASSERT((m_usrIdx2 >= 0) && (m_usrIdx2 < m_boardData.count()));

    if ((m_boardData[m_usrIdx1] == NULL) || (m_boardData[m_usrIdx2] == NULL))
        return false;

    return ((m_boardData[m_usrIdx1]->modifier() == GemCell::HyperCube)
        || (m_boardData[m_usrIdx2]->modifier() == GemCell::HyperCube));
}

void GameBoard::showFloatingScores()
{
    Q_ASSERT(m_textComponent != NULL);
    Q_ASSERT(this->scene() != NULL);

    QListIterator<QDeclarativeItem *> i(m_scoreToShow);
    QDateTime now = QDateTime::currentDateTime();
    while (i.hasNext()) {
        QDeclarativeItem *item = i.next();
        setScore(score() + item->property("scoreValue").toInt());
        item->setProperty("animationStarted", true);
        m_zombieItems.append(qMakePair(now, item));
    }
    m_scoreToShow.clear();
}

/* This function creates score item for combo. Parameters are row and column of last cell in combo,
direction of combo and total count of gems in combo */
void GameBoard::addScoreItem(int row, int column, int gemType, Direction dir, int cnt)
{
    if (!m_engineInited)
        initEngine();

    m_comboCount++;

    /* Adding floating score to show */
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(m_textComponent->create());
    item->setParentItem(this);

    /* Here we set coordinates of score text item. They should be one row higher than combo row to
    make them easily distinguishable */
    if (dir == Row) {
        item->setX( (column - ceil(cnt*1.0/2))*cellSize() );
        item->setY( (row - 1)*cellSize() );
    } else {
        item->setX(column*cellSize());
        item->setY( (row - cnt - 1)*cellSize() );
    }

    item->setProperty("type", gemType);
    int scoreValue = m_comboCount + cnt - 3;

    while (cnt > 0) {
        int nextIdx;
        if (dir == Row) {
            nextIdx = index(row, column - cnt);
        } else {
            nextIdx = index(row - cnt, column);
        }
        if (m_boardData[nextIdx]->modifier() == GemCell::Explosive)
            scoreValue += 2;
        if (m_boardData[nextIdx]->modifier() == GemCell::RowColumnRemove)
            scoreValue += 4;
        cnt--;
    }

    item->setProperty("scoreValue", scoreValue*5*(level() + 1));
    m_scoreToShow.append(item);
}

/* This function creates HyperCube score item. HyperCube scores should be processed in special way
so they were moved in special function  */
void GameBoard::addHyperCubeScoreItem(int row, int column, int gemType)
{
    /* Creating floating score to show */
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(m_textComponent->create());
    item->setParentItem(this);

    /* Here we set coordinates of score text item. They should be one row higher than combo row to
    make them easily distinguishable */
    item->setX( column*cellSize() );
    item->setY( (row - 1)*cellSize() );
    item->setProperty("type", gemType);
    item->setProperty("scoreValue", HYPER_CUBE_MULTIPLYER*5*(level() + 1));
    m_scoreToShow.append(item);
}

/* Convenience function to get (row, column) element */
GemCell * GameBoard::board(int row, int column)
{
    Q_ASSERT(cellInBoard(row, column));
    return m_boardData[index(row, column)];
}

void GameBoard::setLevel(int level)
{
    if (m_level != level) {
        m_level = level;
        m_currentLevelCap = levelCap(level);
        emit levelChanged();
    }
}

/* Saves existing modifiers for gems in boards. Should be used when there is a level up to restore
any modififers user had. */
void GameBoard::saveGemModifiers()
{
    m_boardModifiers[GemCell::Explosive] = 0;
    m_boardModifiers[GemCell::RowColumnRemove] = 0;
    m_boardModifiers[GemCell::HyperCube] = 0;

    for (int i = 0; i < m_boardData.count(); ++i) {
        GemCell *curCell = m_boardData[i];
        if ((curCell != NULL) && (curCell->modifier() != GemCell::Normal))
            ++m_boardModifiers[curCell->modifier()];
    }
}

/* This function restores modifiers for game board, which was previously saved via
saveGemModififers() function. Board for which gem modofoers are restored should not contain any
NULL values */
void GameBoard::restoreGemModifiers()
{
    for (int i = 0; i < m_boardData.count(); ++i) {
        if (m_boardData[i] == NULL)
            return;
    }

    Q_ASSERT(m_boardModifiers[GemCell::Explosive] + m_boardModifiers[GemCell::RowColumnRemove]
        + m_boardModifiers[GemCell::Explosive] <= m_rowCount*m_columnCount);

    restoreModifier(GemCell::Explosive);
    restoreModifier(GemCell::RowColumnRemove);
    restoreModifier(GemCell::HyperCube);
}

/* Restores one modifier to the gems in board. There are no checks for numberOfGemsWithNoModifiers
< numberOfModifiers, so it should be done on the caller's side */
void GameBoard::restoreModifier(GemCell::Modifier modifier)
{
    if (modifier == GemCell::Normal)
        return;

    Q_ASSERT(m_boardModifiers.contains(modifier));

    while (m_boardModifiers[modifier] > 0) {
        int cellNumber = floor(rand() % m_boardData.count());
        if (m_boardData[cellNumber]->modifier() == GemCell::Normal) {
            m_boardData[cellNumber]->setModifier(modifier);
            --m_boardModifiers[modifier];
        }
    }
}

/* Saving game board state to simple string representation. Only board is saved. Score and level
should be saved separately. */
QString GameBoard::toString()
{
    QString result;
    for (int row = 0; row < m_rowCount; ++row) {
        for (int column = 0; column < m_columnCount; ++column) {
            if (board(row, column) == NULL) {
                result += "-- ";
            } else {
                switch (board(row, column)->modifier()) {
                case GemCell::Explosive:
                    result += "E";
                    break;
                case GemCell::RowColumnRemove:
                    result += "R";
                    break;
                case GemCell::HyperCube:
                    result += "H";
                    break;
                default:
                    result += " ";
                }
                result += board(row, column)->property("type").toString() + " ";
            }
        }
        result += "\n";
    }
    return result;
}

/* Restoring game board state, which was previously saved with toString() */
void GameBoard::fromString(QString str)
{
    clearBoard();

    QStringList gemStrings = str.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (gemStrings.count() < m_rowCount*m_columnCount) {
        qDebug() << "Too few gemStrings" << gemStrings.count();
        return;
    }

    for (int row = 0; row < m_rowCount; ++row) {
        for (int column = 0; column < m_columnCount; ++column) {
            int curIdx = index(row, column);
            QString curGemStr = gemStrings[curIdx];
            if (curGemStr.length() < 2) {
                GemCell *curBlock = createBlock(row, column);
                curBlock->setProperty("type", curGemStr);
            } else if (curGemStr != "--") {
                GemCell *curBlock = createBlock(row, column);
                curBlock->setProperty("type", QString(curGemStr[1]));
                if (curGemStr[0] == 'R') {
                    curBlock->setModifier(GemCell::RowColumnRemove);
                } else if (curGemStr[0] == 'H') {
                    curBlock->setModifier(GemCell::HyperCube);
                } else if (curGemStr[0] == 'E') {
                    curBlock->setModifier(GemCell::Explosive);
                }
            }
        }
    }
}

/* Saving board state to file (with predefined name). All information (score, level, gems in board)
is saved. */
void GameBoard::saveBoardStateToFile()
{
    QFile outFile("save.board");
    if (outFile.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream outStream(&outFile);
        outStream << score() << "\n";
        outStream << level() << "\n";
        outStream << toString();
        outFile.close();
    }
}

/* Loading board state from file (with predefined name) */
void GameBoard::loadBoardStateFromFile()
{
    QFile inFile("save.board");
    if (inFile.open(QFile::ReadOnly)) {
        QTextStream inStream(&inFile);
        QString strScore, strLevel, strBoardLine, strBoard;
        inStream >> strScore;
        inStream >> strLevel;
        do {
            strBoardLine = inStream.readLine();
            strBoard += strBoardLine;
        } while (!strBoardLine.isNull());

        bool ok;

        setLevel(strLevel.toInt(&ok));
        Q_ASSERT(ok);
        setScore(strScore.toInt(&ok));
        Q_ASSERT(ok);
        fromString(strBoard);

        m_gameStarted = true;

        if (inFile.remove()) {
            qDebug() << "[loadBoardStateFromFile] Save file removed";
        } else {
            qDebug() << "[loadBoardStateFromFile] ERROR: Can't remove save file";
        }
    }
}

/* Method for getting level cap for given level */
int GameBoard::levelCap(int level)
{
    /* 5*(1+1)+5*(2+1)+...+5*(level+1) = 5*(1+..+level)+5*level = 5*level*(level+1)/2 + 5*level
    = 5*level*(level+3)/2 */
    return (5*level*(level + 3)/2*LEVEL_CAP_MULTIPLYER*pow(DIFFICULTY_MULTIPLYER, level - 1));
}

/* Method for finding any available combo for current game board state. If hintIdx parameter is
passes it is set to first found possible combo. We are doing it via swapping gems in board and
testing for new combos after every swap. */
bool GameBoard::hasPossibleCombos(int *hintIdx)
{
    bool result = false;
    /* As we're going to swap to the right and to the bottom add -1 to column/row count */
    for (int row = 0; row < m_rowCount; ++row) {
        for (int column = 0; column < m_columnCount; ++column) {
            /* Checking object for hypercube modififer, which automatically means possible combo */
           GemCell *cell = safeGetCell(row, column);
           if ((cell != NULL) && (cell->modifier() == GemCell::HyperCube)) {
               if (hintIdx != NULL)
                   *hintIdx = index(row, column);
               return true;
           }

            /* Swapping right */
            if (column != m_columnCount - 1) {
                m_boardData.swap(index(row, column), index(row, column + 1));

                if (hasRowCombo(row, column) || hasColumnCombo(row, column)) {
                    result = true;
                    /* If combo is here then OTHER cell should be mark with hint */
                    if (hintIdx != NULL)
                        *hintIdx = index(row, column + 1);
                }

                if (hasRowCombo(row, column + 1) || hasColumnCombo(row, column + 1)) {
                    result = true;
                    /* If combo is here then OTHER cell should be mark with hint */
                    if (hintIdx != NULL)
                        *hintIdx = index(row, column);
                }

                m_boardData.swap(index(row, column), index(row, column + 1));
                if (result)
                    return true;
            }

            /* Swapping bottom */
            if (row != m_rowCount - 1) {
                m_boardData.swap(index(row, column), index(row + 1, column));

                if (hasRowCombo(row, column) || hasColumnCombo(row, column)) {
                    result = true;
                    /* If combo is here then OTHER cell should be mark with hint */
                    if (hintIdx != NULL)
                        *hintIdx = index(row + 1, column);
                }

                if (hasRowCombo(row + 1, column) || hasColumnCombo(row + 1, column)) {
                    result = true;
                    /* If combo is here then OTHER cell should be mark with hint */
                    if (hintIdx != NULL)
                        *hintIdx = index(row, column);
                }

                m_boardData.swap(index(row, column), index(row + 1, column));
                if (result)
                    return true;
            }
        }
    }

    return false;
}

void GameBoard::showHint()
{
    int hintIdx;
    if (hasPossibleCombos(&hintIdx)) {
        setProperty("hintX", hintIdx % 8 * cellSize());
        setProperty("hintY", hintIdx / 8 * cellSize());
        setProperty("hintVisible", true);
    }
}

/* Checks if there is row combo containing (row, column) cell */
bool GameBoard::hasRowCombo(int row, int column)
{
    int type = safeGetCellType(row, column);
    if (type == -1)
        return false;
    bool hasCombo1 = (safeGetCellType(row, column - 2) == type)
        && (safeGetCellType(row, column - 1) == type);
    bool hasCombo2 = (safeGetCellType(row, column - 1) == type)
        && (safeGetCellType(row, column + 1) == type);
    bool hasCombo3 = (safeGetCellType(row, column + 1) == type)
        && (safeGetCellType(row, column + 2) == type);
    return (hasCombo1 || hasCombo2 || hasCombo3);
}


/* Checks if there is column combo containing (row, column) cell */
bool GameBoard::hasColumnCombo(int row, int column)
{
    int type = safeGetCellType(row, column);
    if (type == -1)
        return false;
    bool hasCombo1 = (safeGetCellType(row - 2, column) == type)
        && (safeGetCellType(row - 1, column) == type);
    bool hasCombo2 = (safeGetCellType(row - 1, column) == type)
        && (safeGetCellType(row + 1, column) == type);
    bool hasCombo3 = (safeGetCellType(row + 1, column) == type)
        && (safeGetCellType(row + 2, column) == type);
    return (hasCombo1 || hasCombo2 || hasCombo3);
}

/* Function for finding combos in gameboard without marking gems as need to be removed */
bool GameBoard::findCombos()
{
    for (int row = 0; row < m_rowCount; ++row) {
        if (findCombosInLine(row, GameBoard::Row))
            return true;
    }

    for (int column = 0; column < m_columnCount; ++column) {
        if (findCombosInLine(column, GameBoard::Column))
            return true;
    }

    return false;
}

/* Finding combos in line without marking gems as need to be removed */
bool GameBoard::findCombosInLine(int lineIndex, Direction direction)
{
    int cnt = 0;
    int lastType = -1;
    int stopValue;
    if (direction == Row) {
        stopValue = m_columnCount;
    } else {
        stopValue = m_rowCount;
    }

    for (int i = 0; i < stopValue; ++i) {
        GemCell *cell;

        /* Cell index differs for different directions */
        if (direction == Row) {
            cell = board(lineIndex, i);
        } else {
            cell = board(i, lineIndex);
        };
        int type = -1;
        if (cell != NULL)
            type = cell->property("type").toInt();

        /* To increase count cell must be not NULL and has the same type as previous */
        if ((cell != NULL) && (type == lastType))
            cnt++;

        /* Checking for combo anyway, if cnt >= 3 we're as good as done */
        if (cnt >= 3)
            return true;

        /* Reset counter if needed */
        if ((cell == NULL) || (type != lastType)) {
            cnt = 1;
            lastType = type;
        }
    }

    return false;
}

/* Function for graphical effect of dropping gems down. Used when there are no more moves or on
level up. Gems are neither removed from nor deleted. This method is only for animation. */
void GameBoard::dropGemsDown()
{
//    QDateTime now = QDateTime::currentDateTime();
    for (int column = 0; column < m_columnCount; ++column) {
        int pause = 0;
        for (int row = m_rowCount - 1; row >= 0; --row) {
            if (board(row, column) != NULL) {
                pause += rand() % 100;
                board(row, column)->setProperty("behaviorPause", pause);
                board(row, column)->setProperty("y", (row + m_rowCount + 1)*cellSize());
//                m_zombieItems.append(qMakePair(now, (QDeclarativeItem *)board(row, column)));
//                setCell(row, column, NULL);
            }
        }
    }
}

/* Function to start new game. Resets board, score and level */
void GameBoard::newGame()
{
    setLevel(1);
    setScore(0);
    resetBoard();
    m_gameStarted = true;
    m_gameLost = false;
}

/* Checks for saved game file and return true if file found */
bool GameBoard::hasSave()
{
    return QFile::exists("save.board");
}

/* Setter method for "cellSize" property */
void GameBoard::setCellSize(int newValue)
{
    if (newValue != m_cellSize) {
        m_cellSize = newValue;
        emit cellSizeChanged();
        if (m_engine != NULL)
            m_engine->rootContext()->setContextProperty("g_scaleFactor", newValue*1.0/SMALL_CELL_SIZE);
        repositionGems();
    }
}

/* Method for resizing and repositioning gems when cellSize() have changed */
void GameBoard::repositionGems()
{
    for (int row = 0; row < m_rowCount; ++row) {
        for (int column = 0; column < m_columnCount; ++column) {
            if (board(row, column) != NULL) {
                board(row, column)->setWidth(cellSize());
                board(row, column)->setHeight(cellSize());
                board(row, column)->setX(column*cellSize());
                board(row, column)->setY(row*cellSize());
                board(row, column)->setProperty("srcSize", cellSize());
            }
        }
    }
}

/* Method for getting settings. It should be used in QML to get settings information from GameBoard
instance */
QVariant GameBoard::settings(QString settingKey)
{
    if (!settingKey.isEmpty()) {
        QSettings gameSettings("gameSettings.ini", QSettings::IniFormat);
        return gameSettings.value(settingKey);
    } else {
        return QString();
    }
}

/* Checking and setting new high score. Method is called when game is lost. */
void GameBoard::checkForHighScore()
{
    QSettings gameSettings("gameSettings.ini", QSettings::IniFormat);
    if (score() >= gameSettings.value("classic/highScore").toInt()) {
        gameSettings.setValue("classic/highScore", score());
        gameSettings.sync();
    }
}
