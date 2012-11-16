#include "GemCell.h"

GemCell::GemCell(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    m_modifier = Normal;
    m_shouldBeRemoved = false;
    m_explodedOnce = false;
    m_timeToDie = false;
    m_behaviorEnabled = true;
    m_invincible = false;
}

void GemCell::setModifier(Modifier newValue)
{
    if (newValue != m_modifier) {
        m_modifier = newValue;
        emit modifierChanged();
    }
}
