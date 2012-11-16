#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include <QList>
#include <QDeclarativeView>
#include <QDeclarativeComponent>
#include <QTimer>
#include <QPair>
#include <QDateTime>
#include "GemCell.h"

class GameBoard: public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(int level READ level WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(int score READ score WRITE setScore NOTIFY scoreChanged)
    Q_PROPERTY(int selGemRow READ selGemRow WRITE setSelGemRow NOTIFY selGemRowChanged)
    Q_PROPERTY(int selGemColumn READ selGemColumn WRITE setSelGemColumn NOTIFY selGemColumnChanged)
    Q_PROPERTY(bool gemSelected READ gemSelected WRITE setGemSelected NOTIFY gemSelectedChanged)
    Q_PROPERTY(bool gameLost READ gameLost)
    Q_PROPERTY(int cellSize READ cellSize WRITE setCellSize NOTIFY cellSizeChanged)
public:
    GameBoard(QDeclarativeItem *parent = NULL);
    ~GameBoard();

    GemCell *operator() (int row, int column) { return m_boardData[row*m_columnCount + column]; }
    void setCell(int row, int column, GemCell *value);

    int rowCount() { return m_rowCount; }
    int columnCount() { return m_columnCount; }

    int level() { return m_level; }
    void setLevel(int level);

    int score() { return m_score; }
    void setScore(int score);

    int selGemRow() { return m_selGemRow; }
    void setSelGemRow(int row);

    int selGemColumn() { return m_selGemColumn; }
    void setSelGemColumn(int column);

    int gemSelected() { return m_gemSelected; }
    void setGemSelected(bool newValue);

    int cellSize() { return m_cellSize; }
    void setCellSize(int newValue);

    bool gameLost() { return m_gameLost; }

    Q_INVOKABLE void newGame();
    Q_INVOKABLE void clearBoard();
    Q_INVOKABLE void resetBoard();
    Q_INVOKABLE bool markCombos();
    Q_INVOKABLE void removeCombos();
    Q_INVOKABLE void shuffleDown();
    Q_INVOKABLE void fillBoard();
    Q_INVOKABLE void removeAll();
    Q_INVOKABLE void loadTestBoard();
    Q_INVOKABLE void handleClick(int x, int y);
    Q_INVOKABLE void dbgPrintGemPositions();
    Q_INVOKABLE void loadBoardStateFromFile();
    Q_INVOKABLE void saveBoardStateToFile();
    Q_INVOKABLE int levelCap(int level);
    Q_INVOKABLE void showHint();
    Q_INVOKABLE bool hasSave();
    Q_INVOKABLE QVariant settings(QString settingKey);

public slots:
    Q_INVOKABLE void dropGemsDown();

signals:
    void levelChanged();
    void scoreChanged();
    void selGemRowChanged();
    void selGemColumnChanged();
    void gemSelectedChanged();
    void cellSizeChanged();
    void levelUp();
    void noMoreMoves();

private slots:
    void checkGemPositions();

private:
    enum Direction {
        Row,
        Column
    };

    bool cellInBoard(int row, int column);
    GemCell * createBlock(int row, int column, int startRow = -1);
    int generateCellType();
    void removeZombies();
    int index(int row, int column);
    int levelFromScore();
    void selectGem(int row, int column);
    void deselectCurrentGem();
    void markExplosions();
    void markIntersections();
    void markBonusGems();
    bool removeExplosions();
    void explodeGem(int row, int column);
    void resetInvincibleStatus();
    void switchBack();
    GemCell * board(int row, int column);
    GemCell * safeGetCell(int row, int column);
    int safeGetCellType(int row, int column);
    void switchGems(int idx1, int idx2);
    bool hyperCubeUsed();
    void showFloatingScores();
    void addScoreItem(int row, int column, int gemType, Direction dir, int cnt);
    void addHyperCubeScoreItem(int row, int column, int gemType);
    void saveGemModifiers();
    void restoreGemModifiers();
    void restoreModifier(GemCell::Modifier modifier);
    QString toString();
    void fromString(QString str);
    bool markCombosInLine(int lineIndex, Direction direction);
    bool hasPossibleCombos(int *hintIdx = NULL);
    bool hasRowCombo(int row, int column);
    bool hasColumnCombo(int row, int column);
    bool findCombos();
    bool findCombosInLine(int lineIndex, Direction direction);
    void initEngine();
    void repositionGems();
    void checkForHighScore();

    QList<GemCell *> m_boardData;
    QList<QPair<QDateTime, QDeclarativeItem *> > m_zombieItems;
    QList<QDeclarativeItem *> m_scoreToShow;
    QMap<GemCell::Modifier, int> m_boardModifiers;
    int m_rowCount;
    int m_columnCount;
    int m_currentStepDelay;

    int m_level;
    int m_score;

    QGraphicsScene *m_scene;
    QDeclarativeComponent *m_component;
    QDeclarativeComponent *m_textComponent;
    QDeclarativeEngine *m_engine;
    GemCell *m_selectedGem;
    bool m_gemSelected;
    int m_selGemRow;
    int m_selGemColumn;
    bool m_gemMovedByUser;
    int m_currentLevelCap;
    bool m_userInteractionAccepted;
    bool m_gameStarted;
    bool m_gameLost;
    int m_cellSize;

    int m_usrIdx1;
    int m_usrIdx2;

    int m_comboCount;
    bool m_engineInited;

    QTimer m_timer;
};

#endif // GAMEBOARD_H
