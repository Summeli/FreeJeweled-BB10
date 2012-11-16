#ifndef GEMCELL_H
#define GEMCELL_H

#include <QDeclarativeItem>

class GemCell : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(bool behaviorEnabled READ behaviorEnabled WRITE setBehaviorEnabled NOTIFY behaviorEnabledChanged)
    Q_PROPERTY(bool timeToDie READ timeToDie WRITE setTimeToDie NOTIFY timeToDieChanged)
    Q_PROPERTY(bool shouldBeRemoved READ shouldBeRemoved WRITE setShouldBeRemoved NOTIFY shouldBeRemovedChanged)
    Q_PROPERTY(bool explodedOnce READ explodedOnce WRITE setExplodedOnce NOTIFY explodedOnceChanged)
    Q_PROPERTY(bool invincible READ invincible WRITE setInvincible)
    Q_PROPERTY(Modifier modifier READ modifier WRITE setModifier NOTIFY modifierChanged)
    Q_ENUMS(Modifier)
public:
    enum Modifier {
        Normal,
        Explosive,
        HyperCube,
        RowColumnRemove
    };

    explicit GemCell(QDeclarativeItem *parent = 0);
    ~GemCell() {}

    bool behaviorEnabled() const { return m_behaviorEnabled; }
    void setBehaviorEnabled(bool newValue) { if (newValue != m_behaviorEnabled) { m_behaviorEnabled = newValue; emit behaviorEnabledChanged(); } }

    bool timeToDie() const { return m_timeToDie; }
    void setTimeToDie(bool newValue) { if (newValue != m_timeToDie) { m_timeToDie = newValue; emit timeToDieChanged(); } }

    bool shouldBeRemoved() const { return m_shouldBeRemoved; }
    void setShouldBeRemoved(bool newValue) { if (newValue != m_shouldBeRemoved) { m_shouldBeRemoved = newValue; emit shouldBeRemovedChanged(); } }

    bool explodedOnce() const { return m_explodedOnce; }
    void setExplodedOnce(bool newValue) { if (newValue != m_explodedOnce) { m_explodedOnce = newValue; emit explodedOnceChanged(); } }

    bool invincible() { return m_invincible; }
    void setInvincible(bool newValue) { if (m_invincible != newValue) m_invincible = newValue; }

    Modifier modifier() { return m_modifier; }
    void setModifier(Modifier newValue);

signals:
    void behaviorEnabledChanged();
    void timeToDieChanged();
    void shouldBeRemovedChanged();
    void explodedOnceChanged();
    void modifierChanged();

private:
    bool m_behaviorEnabled;
    bool m_timeToDie;
    bool m_shouldBeRemoved;
    bool m_explodedOnce;
    bool m_invincible;
    Modifier m_modifier;
};

#endif // GEMCELL_H
