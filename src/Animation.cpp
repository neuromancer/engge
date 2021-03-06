#include <iostream>
#include "Animation.h"
#include "Object.h"

namespace ng
{
Animation::Animation(const sf::Texture &texture, const std::string &name)
    : _sprite(texture), _name(name), _fps(10), _index(0), _state(AnimState::Play)
{
}

Animation::~Animation() = default;

void Animation::reset()
{
    _index = 0;
    if (_rects.empty())
        return;
    auto &sourceRect = _sourceRects[_index];
    auto size = _sizes[_index];
    _sprite.setTextureRect(_rects[_index]);
    _sprite.setOrigin(sf::Vector2f(size.x / 2.f - sourceRect.left, size.y / 2.f - sourceRect.top));
}

void Animation::play(bool loop)
{
    _loop = loop;
    _state = AnimState::Play;
}

void Animation::update(const sf::Time &elapsed)
{
    if (_state == AnimState::Pause)
        return;

    if (_rects.empty())
        return;

    _time += elapsed;
    if (_time.asSeconds() > (1.f / _fps))
    {
        _time = sf::seconds(0);
        _index = (_index + 1) % _rects.size();

        auto sourceRect = _sourceRects[_index];
        auto size = _sizes[_index];
        _sprite.setTextureRect(_rects[_index]);
        _sprite.setOrigin(sf::Vector2f(size.x / 2.f - sourceRect.left, size.y / 2.f - sourceRect.top));

        updateTrigger();
    }
}

void Animation::updateTrigger()
{
    if (_triggers.empty())
        return;

    auto trigger = _triggers[_index];
    if (trigger.has_value() && _pObject)
    {
        _pObject->trig(*trigger);
    }
}

void Animation::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    target.draw(_sprite, states);
}
} // namespace ng
