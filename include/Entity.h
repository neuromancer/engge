#pragma once
#include "SFML/Graphics.hpp"

namespace ng
{
class Trigger
{
public:
  virtual void trig() = 0;
};

class Entity : public sf::Drawable
{
public:
  virtual void update(const sf::Time &elapsed) {}
  virtual int getZOrder() const = 0;

  void setUsePosition(const sf::Vector2f &pos)
  {
    _usePos = pos;
  }

  sf::Vector2f getUsePosition() const
  {
    return _usePos;
  }

  void setTrigger(int triggerNumber, std::shared_ptr<Trigger> trigger)
  {
    _triggers[triggerNumber] = trigger;
  }

  void trig(int triggerNumber)
  {
    auto it = _triggers.find(triggerNumber);
    if (it != _triggers.end())
    {
      it->second->trig();
    }
  }

private:
  std::map<int, std::shared_ptr<Trigger>> _triggers;
  sf::Vector2f _usePos;
};
} // namespace ng
