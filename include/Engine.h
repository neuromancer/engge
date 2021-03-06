#pragma once
#include "SFML/Graphics.hpp"
#include "squirrel.h"
#include "NonCopyable.h"
#include "Verb.h"

namespace ng
{
class Actor;
class Cutscene;
class DialogManager;
class EngineSettings;
class Function;
class InventoryObject;
class Object;
class Preferences;
class Room;
class ScriptExecute;
class SoundDefinition;
class SoundManager;
class TextureManager;
class Verb;
class VerbExecute;

enum class CursorDirection
{
  None = 0,
  Left = 1,
  Right = 1 << 1,
  Up = 1 << 2,
  Down = 1 << 3,
  Hotspot = 1 << 4
};

enum UseFlag
{
  None = 0,
  UseWith,
  UseOn,
  UseIn
};

class Engine : public NonCopyable
{
public:
  explicit Engine(EngineSettings &settings);
  ~Engine();

  void setCameraAt(const sf::Vector2f &at);
  void moveCamera(const sf::Vector2f &offset);
  sf::Vector2f getCameraAt() const;

  void setWindow(sf::RenderWindow &window);

  TextureManager &getTextureManager();
  EngineSettings &getSettings();

  Room *getRoom();
  void setRoom(Room *room);
  std::wstring getText(int id) const;
  void setFadeAlpha(float fade);
  float getFadeAlpha() const;
  void setFadeColor(sf::Color color);
  sf::Color getFadeColor() const;

  void addActor(std::unique_ptr<Actor> actor);
  void addRoom(std::unique_ptr<Room> room);
  const std::vector<std::unique_ptr<Room>> &getRooms() const;
  void addFunction(std::unique_ptr<Function> function);
  void cutscene(std::unique_ptr<Cutscene> function);
  bool inCutscene() const;
  void cutsceneOverride();

  std::vector<std::unique_ptr<Actor>> &getActors();

  void update(const sf::Time &elapsed);
  void draw(sf::RenderWindow &window) const;

  void setCurrentActor(Actor *pCurrentActor);
  Actor *getCurrentActor();

  void setVerb(int characterSlot, int verbSlot, const Verb &verb);
  void setVerbUiColors(int characterSlot, VerbUiColors colors);
  VerbUiColors &getVerbUiColors(int characterSlot);

  void setInputActive(bool active);
  void inputSilentOff();
  void setInputVerbs(bool on);
  bool getInputActive() const;
  bool getInputVerbs() const;

  void follow(Actor *pActor);
  void setVerbExecute(std::unique_ptr<VerbExecute> verbExecute);
  void setScriptExecute(std::unique_ptr<ScriptExecute> scriptExecute);
  const Verb *getVerb(int id) const;

  void addThread(HSQUIRRELVM thread);
  void stopThread(HSQUIRRELVM thread);
  bool isThreadAlive(HSQUIRRELVM thread) const;

  void startDialog(const std::string &dialog, const std::string &node);
  void execute(const std::string &code);
  SoundDefinition *getSoundDefinition(const std::string &name);
  bool executeCondition(const std::string &code);

  sf::Vector2f getMousePos() const;

  Preferences &getPreferences();
  SoundManager &getSoundManager();
  DialogManager &getDialogManager();

  void addSelectableActor(int index, Actor *pActor);
  void actorSlotSelectable(Actor *pActor, bool selectable);
  void actorSlotSelectable(int index, bool selectable);
  void setUseFlag(UseFlag flag, const InventoryObject *object);
  UseFlag getUseFlag(UseFlag flag) const;
  sf::Time getTime() const;

  void setVm(HSQUIRRELVM vm);
  HSQUIRRELVM getVm() const;

private:
struct Impl;
std::unique_ptr<Impl> _pImpl;
  
};
} // namespace ng
