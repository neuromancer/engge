#include "Entity.h"
#include "Trigger.h"

namespace ng
{
void Entity::update(const sf::Time &elapsed)
{
}

void Entity::setLit(bool isLit)
{
    _isLit = isLit;
}

bool Entity::isLit() const
{
    return _isLit;
}

void Entity::setUsePosition(const sf::Vector2f &pos)
{
    _usePos = pos;
}

void Entity::setPosition(const sf::Vector2f &pos)
{
    _transform.setPosition(pos);
}

sf::Vector2f Entity::getPosition() const
{
    return _transform.getPosition();
}

sf::Vector2f Entity::getDefaultPosition() const
{
    return getPosition();
}

sf::Vector2f Entity::getUsePosition() const
{
    return _usePos;
}

void Entity::setTrigger(int triggerNumber, std::shared_ptr<Trigger> trigger)
{
    _triggers[triggerNumber] = trigger;
}

void Entity::trig(int triggerNumber)
{
    auto it = _triggers.find(triggerNumber);
    if (it != _triggers.end())
    {
        it->second->trig();
    }
}

void Entity::trigSound(const std::string &name)
{
}

void Entity::drawForeground(sf::RenderTarget &target, sf::RenderStates states) const
{
}
} // namespace ng