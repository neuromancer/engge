#pragma once
#include <string>
#include <map>
#include "TextureManager.h"
#include "EngineSettings.h"

namespace ng
{
class SpriteSheet
{
public:
  SpriteSheet();
  ~SpriteSheet() = default;

  void setTextureManager(TextureManager *pTextureManager) { _pTextureManager = pTextureManager; }
  void setSettings(EngineSettings *pSettings) { _pSettings = pSettings; }
  void load(const std::string &name);
  const sf::Texture &getTexture() const { return _texture; }
  bool hasRect(const std::string &name) const;
  sf::IntRect getRect(const std::string &name) const;
  sf::IntRect getSpriteSourceSize(const std::string &name) const;
  sf::Vector2i getSourceSize(const std::string &name) const;

private:
  TextureManager *_pTextureManager;
  EngineSettings *_pSettings;
  sf::Texture _texture;
  std::map<std::string, sf::IntRect> _rects;
  std::map<std::string, sf::IntRect> _spriteSourceSize;
  std::map<std::string, sf::Vector2i> _sourceSize;
};
} // namespace ng
