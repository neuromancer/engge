#include <memory>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <math.h>
#include "ActorIcons.h"
#include "ActorIconSlot.h"
#include "Cutscene.h"
#include "Dialog/DialogManager.h"
#include "Engine.h"
#include "Font.h"
#include "Inventory.h"
#include "InventoryObject.h"
#include "Preferences.h"
#include "Room.h"
#include "Screen.h"
#include "ScriptExecute.h"
#include "SoundManager.h"
#include "SpriteSheet.h"
#include "Text.h"
#include "TextDatabase.h"
#include "Verb.h"
#include "VerbExecute.h"
#include "_NGUtil.h"

namespace ng
{
CursorDirection operator|=(CursorDirection &lhs, CursorDirection rhs)
{
    lhs = static_cast<CursorDirection>(
        static_cast<std::underlying_type<CursorDirection>::type>(lhs) |
        static_cast<std::underlying_type<CursorDirection>::type>(rhs));
    return lhs;
}

bool operator&(CursorDirection lhs, CursorDirection rhs)
{
    return static_cast<CursorDirection>(
               static_cast<std::underlying_type<CursorDirection>::type>(lhs) &
               static_cast<std::underlying_type<CursorDirection>::type>(rhs)) > CursorDirection::None;
}

struct Engine::Impl
{
    Engine *_pEngine;
    EngineSettings &_settings;
    TextureManager _textureManager;
    Room *_pRoom;
    std::vector<std::unique_ptr<Actor>> _actors;
    std::vector<std::unique_ptr<Room>> _rooms;
    std::vector<std::unique_ptr<Function>> _newFunctions;
    std::vector<std::unique_ptr<Function>> _functions;
    std::unique_ptr<Cutscene> _pCutscene;
    sf::Color _fadeColor;
    sf::RenderWindow *_pWindow;
    sf::Vector2f _cameraPos;
    TextDatabase _textDb;
    Font _fntFont;
    Actor *_pCurrentActor;
    std::array<VerbSlot, 6> _verbSlots;
    std::array<VerbUiColors, 6> _verbUiColors;
    bool _inputActive;
    bool _showCursor;
    bool _inputVerbsActive;
    SpriteSheet _verbSheet, _gameSheet;
    nlohmann::json _jsonInventoryItems;
    Actor *_pFollowActor;
    sf::IntRect _verbRects[9];
    Object *_pCurrentObject;
    const InventoryObject *_pUseObject;
    sf::Vector2f _mousePos;
    std::unique_ptr<VerbExecute> _pVerbExecute;
    std::unique_ptr<ScriptExecute> _pScriptExecute;
    const Verb *_pVerb;
    std::vector<HSQUIRRELVM> _threads;
    DialogManager _dialogManager;
    Preferences _preferences;
    SoundManager _soundManager;
    CursorDirection _cursorDirection;
    std::array<ActorIconSlot, 6> _actorsIconSlots;
    UseFlag _useFlag;
    ActorIcons _actorIcons;
    Inventory _inventory;
    HSQUIRRELVM _vm;
    sf::Time _time;
    bool _isMouseDown;

    Impl(EngineSettings &settings);

    sf::IntRect getVerbRect(int id, std::string lang = "en", bool isRetro = false) const;
    void drawVerbs(sf::RenderWindow &window) const;
    void drawCursor(sf::RenderWindow &window) const;
    void drawCursorText(sf::RenderWindow &window) const;
    void drawFade(sf::RenderWindow &window) const;
    void clampCamera();
    int getCurrentActorIndex() const;
    sf::IntRect getCursorRect() const;
    void appendUseFlag(std::wstring &sentence) const;
    bool clickedAt(const sf::Vector2f &pos);
    void updateCutscene(const sf::Time &elapsed);
    void updateFunctions(const sf::Time &elapsed);
    void updateActorIcons(const sf::Time &elapsed);
    void updateMouseCursor();
    void updateCurrentObject(const sf::Vector2f &mousPos);
};

Engine::Impl::Impl(EngineSettings &settings)
    : _pEngine(nullptr),
      _settings(settings),
      _textureManager(settings),
      _fadeColor(0, 0, 0, 255),
      _pWindow(nullptr),
      _pRoom(nullptr),
      _pCutscene(nullptr),
      _pCurrentActor(nullptr),
      _inputActive(false),
      _showCursor(false),
      _inputVerbsActive(false),
      _pFollowActor(nullptr),
      _pCurrentObject(nullptr),
      _pVerb(nullptr),
      _soundManager(settings),
      _useFlag(UseFlag::None),
      _pUseObject(nullptr),
      _cursorDirection(CursorDirection::None),
      _actorIcons(_actorsIconSlots, _verbUiColors, _pCurrentActor),
      _inventory(_actorsIconSlots, _verbUiColors, _pCurrentActor)
{
    _verbSheet.setSettings(&settings);
    _verbSheet.setTextureManager(&_textureManager);
    _gameSheet.setSettings(&settings);
    _gameSheet.setTextureManager(&_textureManager);
}

Engine::Engine(EngineSettings &settings)
    : _pImpl(std::make_unique<Impl>(settings))
{
    time_t t;
    auto seed = (unsigned)time(&t);
    std::cout << "seed: " << seed << std::endl;
    srand(seed);

    _pImpl->_pEngine = this;
    _pImpl->_dialogManager.setEngine(this);
    _pImpl->_actorIcons.setEngine(this);
    _pImpl->_inventory.setEngine(this);
    _pImpl->_fntFont.setTextureManager(&_pImpl->_textureManager);
    _pImpl->_fntFont.setSettings(&settings);
    _pImpl->_fntFont.load("FontModernSheet");

    // load all messages
    _pImpl->_textDb.setSettings(settings);
    _pImpl->_textDb.load("ThimbleweedText_en.tsv");

    _pImpl->_verbSheet.load("VerbSheet");
    _pImpl->_gameSheet.load("GameSheet");

    sf::Vector2f size(Screen::Width / 6.f, Screen::Height / 14.f);
    for (auto i = 0; i < 9; i++)
    {
        auto left = (i / 3) * size.x;
        auto top = Screen::Height - size.y * 3 + (i % 3) * size.y;
        _pImpl->_verbRects[i] = sf::IntRect(left, top, size.x, size.y);
    }
}

Engine::~Engine() = default;

sf::Vector2f Engine::getCameraAt() const { return _pImpl->_cameraPos; }

void Engine::setWindow(sf::RenderWindow &window) { _pImpl->_pWindow = &window; }

TextureManager &Engine::getTextureManager() { return _pImpl->_textureManager; }

EngineSettings &Engine::getSettings() { return _pImpl->_settings; }

Room *Engine::getRoom() { return _pImpl->_pRoom; }

std::wstring Engine::getText(int id) const { return _pImpl->_textDb.getText(id); }

void Engine::setFadeAlpha(float fade) { _pImpl->_fadeColor.a = static_cast<uint8_t>(fade * 255); }

float Engine::getFadeAlpha() const { return _pImpl->_fadeColor.a / 255.f; }

void Engine::setFadeColor(sf::Color color) { _pImpl->_fadeColor = color; }

sf::Color Engine::getFadeColor() const { return _pImpl->_fadeColor; }

void Engine::addActor(std::unique_ptr<Actor> actor) { _pImpl->_actors.push_back(std::move(actor)); }

void Engine::addRoom(std::unique_ptr<Room> room) { _pImpl->_rooms.push_back(std::move(room)); }

const std::vector<std::unique_ptr<Room>> &Engine::getRooms() const { return _pImpl->_rooms; }

void Engine::addFunction(std::unique_ptr<Function> function) { _pImpl->_newFunctions.push_back(std::move(function)); }

std::vector<std::unique_ptr<Actor>> &Engine::getActors() { return _pImpl->_actors; }

Actor *Engine::getCurrentActor() { return _pImpl->_pCurrentActor; }

void Engine::setVerb(int characterSlot, int verbSlot, const Verb &verb) { _pImpl->_verbSlots[characterSlot].setVerb(verbSlot, verb); }

void Engine::setVerbUiColors(int characterSlot, VerbUiColors colors) { _pImpl->_verbUiColors[characterSlot] = colors; }

VerbUiColors &Engine::getVerbUiColors(int characterSlot) { return _pImpl->_verbUiColors[characterSlot]; }

bool Engine::getInputActive() const { return _pImpl->_inputActive; }

bool Engine::getInputVerbs() const { return _pImpl->_inputVerbsActive; }

void Engine::follow(Actor *pActor) { _pImpl->_pFollowActor = pActor; }

void Engine::setVerbExecute(std::unique_ptr<VerbExecute> verbExecute) { _pImpl->_pVerbExecute = std::move(verbExecute); }

void Engine::setScriptExecute(std::unique_ptr<ScriptExecute> scriptExecute) { _pImpl->_pScriptExecute = std::move(scriptExecute); }

void Engine::addThread(HSQUIRRELVM thread) { _pImpl->_threads.push_back(thread); }

sf::Vector2f Engine::getMousePos() const { return _pImpl->_mousePos; }

Preferences &Engine::getPreferences() { return _pImpl->_preferences; }

SoundManager &Engine::getSoundManager() { return _pImpl->_soundManager; }

DialogManager &Engine::getDialogManager() { return _pImpl->_dialogManager; }

UseFlag Engine::getUseFlag(UseFlag flag) const { return _pImpl->_useFlag; }

sf::Time Engine::getTime() const { return _pImpl->_time; }

void Engine::setVm(HSQUIRRELVM vm) { _pImpl->_vm = vm; }

HSQUIRRELVM Engine::getVm() const { return _pImpl->_vm; }

void Engine::setRoom(Room *room)
{
    _pImpl->_fadeColor = sf::Color::Transparent;
    _pImpl->_pRoom = room;
}

void Engine::setInputActive(bool active)
{
    _pImpl->_inputActive = active;
    _pImpl->_showCursor = active;
}

void Engine::inputSilentOff()
{
    _pImpl->_inputActive = false;
}

void Engine::setInputVerbs(bool on)
{
    _pImpl->_inputVerbsActive = on;
}

sf::IntRect Engine::Impl::getVerbRect(int id, std::string lang, bool isRetro) const
{
    std::string s;
    std::string name;
    switch (id)
    {
    case 1:
        name = "walkto";
        break;
    case 2:
        name = "lookat";
        break;
    case 3:
        name = "talkto";
        break;
    case 4:
        name = "pickup";
        break;
    case 5:
        name = "open";
        break;
    case 6:
        name = "close";
        break;
    case 7:
        name = "push";
        break;
    case 8:
        name = "pull";
        break;
    case 9:
        name = "give";
        break;
    case 10:
        name = "use";
        break;
    }
    s.append(name).append(isRetro ? "_retro" : "").append("_").append(lang);
    return _verbSheet.getRect(s);
}

const Verb *Engine::getVerb(int id) const
{
    auto index = _pImpl->getCurrentActorIndex();
    for (auto i = 0; i < 10; i++)
    {
        const auto &verb = _pImpl->_verbSlots[index].getVerb(i);
        if (verb.id == id)
        {
            return &verb;
        }
    }
    return nullptr;
}

void Engine::setCameraAt(const sf::Vector2f &at)
{
    _pImpl->_cameraPos = at;
}

void Engine::moveCamera(const sf::Vector2f &offset)
{
    _pImpl->_cameraPos += offset;
    _pImpl->clampCamera();
}

void Engine::Impl::clampCamera()
{
    if (_cameraPos.x < 0)
        _cameraPos.x = 0;
    if (_cameraPos.y < 0)
        _cameraPos.y = 0;
    if (!_pRoom)
        return;
    const auto &size = _pRoom->getRoomSize();
    if (_cameraPos.x > size.x - Screen::Width)
        _cameraPos.x = size.x - Screen::Width;
    if (_cameraPos.y > size.y - Screen::Height)
        _cameraPos.y = size.y - Screen::Height;
}

void Engine::Impl::updateCutscene(const sf::Time &elapsed)
{
    if (_pCutscene)
    {
        (*_pCutscene)(elapsed);
        if (_pCutscene->isElapsed())
        {
            _pCutscene.release();
        }
    }
}

void Engine::Impl::updateFunctions(const sf::Time &elapsed)
{
    for (auto &function : _newFunctions)
    {
        _functions.push_back(std::move(function));
    }
    _newFunctions.clear();
    for (auto &function : _functions)
    {
        (*function)(elapsed);
    }
    _functions.erase(std::remove_if(_functions.begin(), _functions.end(),
                                    [](std::unique_ptr<Function> &f) { return f->isElapsed(); }),
                     _functions.end());
    for (auto &actor : _actors)
    {
        actor->update(elapsed);
    }
}

void Engine::Impl::updateActorIcons(const sf::Time &elapsed)
{
    _actorIcons.setMousePosition(_mousePos);
    _actorIcons.update(elapsed);
}

void Engine::Impl::updateMouseCursor()
{
    if (_mousePos.x < 20)
        _cursorDirection |= CursorDirection::Left;
    else if (_mousePos.x > Screen::Width - 20)
        _cursorDirection |= CursorDirection::Right;
    if (_mousePos.y < 10)
        _cursorDirection |= CursorDirection::Up;
    else if (_mousePos.y > Screen::Height - 10)
        _cursorDirection |= CursorDirection::Down;
    if (_pCurrentObject)
        _cursorDirection |= CursorDirection::Hotspot;
}

void Engine::Impl::updateCurrentObject(const sf::Vector2f &mousPos)
{
    _pCurrentObject = nullptr;
    const auto &objects = _pRoom->getObjects();
    auto it = std::find_if(objects.cbegin(), objects.cend(), [mousPos](const std::unique_ptr<Object> &pObj) {
        if (!pObj->isTouchable())
            return false;
        auto rect = pObj->getRealHotspot();
        return rect.contains((sf::Vector2i)mousPos);
    });
    if (it != objects.cend())
    {
        _pCurrentObject = it->get();
    }
}

void Engine::update(const sf::Time &elapsed)
{
    auto wasMouseDown = _pImpl->_isMouseDown;
    _pImpl->_isMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    auto isMouseClick = wasMouseDown != _pImpl->_isMouseDown && !_pImpl->_isMouseDown;

    _pImpl->_time += elapsed;

    _pImpl->updateCutscene(elapsed);
    _pImpl->updateFunctions(elapsed);

    if (!_pImpl->_pRoom)
        return;

    _pImpl->_pRoom->update(elapsed);
    if (_pImpl->_pFollowActor)
    {
        auto pos = _pImpl->_pFollowActor->getPosition();
        _pImpl->_cameraPos = pos - sf::Vector2f(Screen::HalfWidth, Screen::HalfHeight);
        _pImpl->clampCamera();
    }

    _pImpl->_mousePos = _pImpl->_pWindow->mapPixelToCoords(sf::Mouse::getPosition(*_pImpl->_pWindow));
    _pImpl->updateActorIcons(elapsed);

    _pImpl->_cursorDirection = CursorDirection::None;
    _pImpl->updateMouseCursor();

    auto mousePosInRoom = _pImpl->_mousePos + _pImpl->_cameraPos;

    _pImpl->updateCurrentObject(mousePosInRoom);

    _pImpl->_inventory.setMousePosition(_pImpl->_mousePos);
    _pImpl->_inventory.update(elapsed);
    _pImpl->_dialogManager.update(elapsed);

    if (!_pImpl->_inputActive)
        return;

    if (!isMouseClick)
        return;

    if (_pImpl->clickedAt(mousePosInRoom))
        return;

    if (_pImpl->_dialogManager.isActive())
        return;

    if (_pImpl->_actorIcons.isMouseOver())
        return;

    // input click on a verb ?
    auto verbId = -1;
    if (_pImpl->_inputVerbsActive)
    {
        for (auto i = 0; i < 9; i++)
        {
            if (_pImpl->_verbRects[i].contains((sf::Vector2i)_pImpl->_mousePos))
            {
                verbId = i;
                break;
            }
        }
    }

    int currentActorIndex = _pImpl->getCurrentActorIndex();
    if (verbId != -1 && currentActorIndex != -1)
    {
        _pImpl->_pVerb = &_pImpl->_verbSlots[currentActorIndex].getVerb(1 + verbId);
        _pImpl->_useFlag = UseFlag::None;
        _pImpl->_pUseObject = nullptr;
    }
    else if (_pImpl->_pVerb && _pImpl->_pVerb->id == 1 && !_pImpl->_pCurrentObject && _pImpl->_pCurrentActor)
    {
        _pImpl->_pCurrentActor->walkTo(mousePosInRoom);
    }
    else if (_pImpl->_pCurrentObject)
    {
        if (_pImpl->_pUseObject)
        {
            _pImpl->_pVerbExecute->use(_pImpl->_pUseObject, _pImpl->_pCurrentObject);
        }
        else
        {
            auto pVerb = _pImpl->_pVerb;
            if (pVerb && pVerb->id == 1)
            {
                pVerb = getVerb(_pImpl->_pCurrentObject->getDefaultVerb());
            }
            _pImpl->_pVerbExecute->execute(_pImpl->_pCurrentObject, pVerb);
        }
    }
    else if (_pImpl->_inventory.getCurrentInventoryObject())
    {
        auto pVerb = _pImpl->_pVerb;
        auto pInventoryObj = _pImpl->_inventory.getCurrentInventoryObject();
        if (!pVerb || pVerb->id == 1)
        {
            pVerb = getVerb(pInventoryObj->getDefaultVerb());
        }
        _pImpl->_pVerbExecute->execute(pInventoryObj, pVerb);
    }
    else if (currentActorIndex != -1)
    {
        _pImpl->_pVerb = getVerb(1);
        _pImpl->_useFlag = UseFlag::None;
        _pImpl->_pUseObject = nullptr;
    }
}

void Engine::setCurrentActor(Actor *pCurrentActor)
{
    _pImpl->_pCurrentActor = pCurrentActor;
    if (_pImpl->_pCurrentActor)
    {
        follow(_pImpl->_pCurrentActor);
    }
}

bool Engine::Impl::clickedAt(const sf::Vector2f &pos)
{
    if (!_pRoom)
        return false;

    auto pTable = _pRoom->getTable();
    sq_pushobject(_vm, *pTable);
    sq_pushstring(_vm, _SC("clickedAt"), -1);
    if (SQ_SUCCEEDED(sq_get(_vm, -2)))
    {
        sq_remove(_vm, -2);
        sq_pushobject(_vm, *pTable);
        sq_pushinteger(_vm, pos.x);
        sq_pushinteger(_vm, pos.y);
        sq_call(_vm, 3, SQTrue, SQTrue);
        SQInteger handled = 0;
        sq_getinteger(_vm, -1, &handled);
        return handled == 1;
    }
    return false;
}

void Engine::draw(sf::RenderWindow &window) const
{
    if (!_pImpl->_pRoom)
        return;

    _pImpl->_pRoom->draw(window, _pImpl->_cameraPos);

    window.draw(_pImpl->_dialogManager);

    if (!_pImpl->_dialogManager.isActive() && _pImpl->_inputActive)
    {
        _pImpl->drawVerbs(window);
        window.draw(_pImpl->_inventory);
        window.draw(_pImpl->_actorIcons);
    }

    _pImpl->drawFade(window);
    _pImpl->drawCursor(window);
    _pImpl->drawCursorText(window);
}

void Engine::Impl::drawFade(sf::RenderWindow &window) const
{
    sf::RectangleShape fadeShape;
    fadeShape.setSize(sf::Vector2f(Screen::Width, Screen::Height));
    fadeShape.setFillColor(_fadeColor);
    window.draw(fadeShape);
}

void Engine::Impl::drawCursor(sf::RenderWindow &window) const
{
    if (!_inputActive)
        return;

    auto cursorSize = sf::Vector2f(68.f * Screen::Width / 1284, 68.f * Screen::Height / 772);
    sf::RectangleShape shape;
    shape.setPosition(_mousePos);
    shape.setOrigin(cursorSize / 2.f);
    shape.setSize(cursorSize);
    shape.setTexture(&_gameSheet.getTexture());

    shape.setTextureRect(getCursorRect());
    window.draw(shape);
}

sf::IntRect Engine::Impl::getCursorRect() const
{
    const auto &size = _pRoom->getRoomSize();
    if (_cursorDirection & CursorDirection::Left && _cameraPos.x > 0)
    {
        return _cursorDirection & CursorDirection::Hotspot ? _gameSheet.getRect("hotspot_cursor_left") : _gameSheet.getRect("cursor_left");
    }
    if (_cursorDirection & CursorDirection::Right && _cameraPos.x < size.x - Screen::Width)
    {
        return _cursorDirection & CursorDirection::Hotspot ? _gameSheet.getRect("hotspot_cursor_right") : _gameSheet.getRect("cursor_right");
    }
    if (_cursorDirection & CursorDirection::Up && _cameraPos.y > 0)
    {
        return _cursorDirection & CursorDirection::Hotspot ? _gameSheet.getRect("hotspot_cursor_back") : _gameSheet.getRect("cursor_back");
    }
    if (_cursorDirection & CursorDirection::Down && _cameraPos.y < size.y - Screen::Height)
    {
        return _cursorDirection & CursorDirection::Hotspot ? _gameSheet.getRect("hotspot_cursor_front") : _gameSheet.getRect("cursor_front");
    }
    return _cursorDirection & CursorDirection::Hotspot ? _gameSheet.getRect("hotspot_cursor") : _gameSheet.getRect("cursor");
}

void Engine::Impl::drawCursorText(sf::RenderWindow &window) const
{
    if (!_inputActive)
        return;

    auto pVerb = _pVerb;
    if (!pVerb)
        pVerb = _pEngine->getVerb(1);

    NGText text;
    text.setFont(_fntFont);
    text.setColor(sf::Color::White);

    if (_pCurrentObject)
    {
        if (pVerb->id == 1)
        {
            pVerb = _pEngine->getVerb(_pCurrentObject->getDefaultVerb());
        }

        std::wstring s;
        if (!pVerb->text.empty())
        {
            auto id = std::strtol(pVerb->text.substr(1).data(), nullptr, 10);
            s.append(_pEngine->getText(id));
        }
        if (_pUseObject)
        {
            s.append(L" ").append(_pUseObject->getName());
        }
        else
        {
            s.append(L" ").append(_pCurrentObject->getName());
        }
        appendUseFlag(s);
        if (_pUseObject)
        {
            s.append(L" ").append(_pCurrentObject->getName());
        }
        text.setText(s);

        sf::RenderStates states;
        states.transform.translate(-_cameraPos);
        _pCurrentObject->drawHotspot(window, states);
    }
    else
    {
        auto pInventoryObj = _inventory.getCurrentInventoryObject();
        if (pVerb->id == 1 && pInventoryObj)
        {
            pVerb = _pEngine->getVerb(pInventoryObj->getDefaultVerb());
        }
        auto id = std::strtol(pVerb->text.substr(1).data(), nullptr, 10);
        std::wstring s;
        s.append(_pEngine->getText(id));
        if (pInventoryObj)
        {
            s.append(L" ").append(pInventoryObj->getName());
        }
        appendUseFlag(s);
        text.setText(s);
    }

    auto y = _mousePos.y - 22 < 8 ? _mousePos.y + 8 : _mousePos.y - 22;
    if (y < 0)
        y = 0;
    auto x = std::clamp((int)_mousePos.x, 20, Screen::Width - 20 - (int)text.getBoundRect().width / 2);
    text.setPosition(x, y);
    window.draw(text, sf::RenderStates::Default);
}

void Engine::Impl::appendUseFlag(std::wstring &sentence) const
{
    switch (_useFlag)
    {
    case UseFlag::UseWith:
        sentence.append(L" ").append(_pEngine->getText(10000));
        break;
    case UseFlag::UseIn:
        sentence.append(L" ").append(_pEngine->getText(10002));
        break;
    case UseFlag::UseOn:
        sentence.append(L" ").append(_pEngine->getText(10001));
        break;
    case UseFlag::None:
        break;
    }
}

int Engine::Impl::getCurrentActorIndex() const
{
    for (auto i = 0; i < _actorsIconSlots.size(); i++)
    {
        const auto &selectableActor = _actorsIconSlots[i];
        if (selectableActor.pActor == _pCurrentActor)
        {
            return i;
        }
    }
    return -1;
}

void Engine::Impl::drawVerbs(sf::RenderWindow &window) const
{
    if (!_inputVerbsActive)
        return;

    int currentActorIndex = getCurrentActorIndex();
    if (!_inputActive || currentActorIndex == -1 || _verbSlots[currentActorIndex].getVerb(0).id == 0)
        return;

    auto verbId = -1;
    if (_pCurrentObject)
    {
        auto defaultVerb = _pCurrentObject->getDefaultVerb();
        verbId = _verbSlots[currentActorIndex].getVerbIndex(defaultVerb);
    }
    else
    {
        for (auto i = 0; i < 9; i++)
        {
            if (_verbRects[i].contains((sf::Vector2i)_mousePos))
            {
                verbId = i;
                break;
            }
        }
    }

    sf::RenderStates states;
    states.texture = &_verbSheet.getTexture();

    sf::Vector2f size(Screen::Width / 6.f, Screen::Height / 14.f);
    auto ratio = sf::Vector2f(Screen::Width / 1280.f, Screen::Height / 720.f);
    for (int x = 0; x < 3; x++)
    {
        auto maxW = 0;
        for (int y = 0; y < 3; y++)
        {
            auto verb = _verbSlots[currentActorIndex].getVerb(x * 3 + y + 1);
            auto rect = getVerbRect(verb.id);
            maxW = fmax(maxW, rect.width * ratio.x);
        }
        auto padding = (size.x - maxW) / 2.f;
        auto left = padding + x * size.x;

        for (int y = 0; y < 3; y++)
        {
            auto top = Screen::Height - size.y * 3 + y * size.y;
            auto verb = _verbSlots[currentActorIndex].getVerb(x * 3 + y + 1);
            auto rect = getVerbRect(verb.id);
            auto verbSize = sf::Vector2f(rect.width * ratio.x, rect.height * ratio.y);
            int index = x * 3 + y;
            auto color = index == verbId ? _verbUiColors[currentActorIndex].verbHighlight : _verbUiColors[currentActorIndex].verbNormalTint;
            sf::RectangleShape verbShape;
            verbShape.setFillColor(color);
            verbShape.setPosition(left, top);
            verbShape.setSize(verbSize);
            verbShape.setTexture(&_verbSheet.getTexture());
            verbShape.setTextureRect(rect);
            window.draw(verbShape);
        }
    }
}

bool Engine::isThreadAlive(HSQUIRRELVM thread) const
{
    return std::find(_pImpl->_threads.begin(), _pImpl->_threads.end(), thread) != _pImpl->_threads.end();
}

void Engine::startDialog(const std::string &dialog, const std::string &node)
{
    _pImpl->_dialogManager.start(dialog, node);
}

void Engine::execute(const std::string &code)
{
    _pImpl->_pScriptExecute->execute(code);
}

SoundDefinition *Engine::getSoundDefinition(const std::string &name)
{
    return _pImpl->_pScriptExecute->getSoundDefinition(name);
}

bool Engine::executeCondition(const std::string &code)
{
    return _pImpl->_pScriptExecute->executeCondition(code);
}

void Engine::stopThread(HSQUIRRELVM thread)
{
    auto it = std::find(_pImpl->_threads.begin(), _pImpl->_threads.end(), thread);
    if (it == _pImpl->_threads.end())
        return;
    _pImpl->_threads.erase(it);
}

void Engine::addSelectableActor(int index, Actor *pActor)
{
    _pImpl->_actorsIconSlots[index - 1].selectable = true;
    _pImpl->_actorsIconSlots[index - 1].pActor = pActor;
}

void Engine::actorSlotSelectable(Actor *pActor, bool selectable)
{
    for (auto &selectableActor : _pImpl->_actorsIconSlots)
    {
        if (selectableActor.pActor == pActor)
        {
            selectableActor.selectable = selectable;
            return;
        }
    }
}

void Engine::actorSlotSelectable(int index, bool selectable)
{
    _pImpl->_actorsIconSlots[index].selectable = selectable;
}

void Engine::setUseFlag(UseFlag flag, const InventoryObject *object)
{
    _pImpl->_useFlag = flag;
    _pImpl->_pUseObject = object;
}

void Engine::cutsceneOverride()
{
    if (!_pImpl->_pCutscene)
        return;
    _pImpl->_pCutscene->cutsceneOverride();
}

void Engine::cutscene(std::unique_ptr<Cutscene> function)
{
    _pImpl->_pCutscene = std::move(function);
}

bool Engine::inCutscene() const
{
    return _pImpl->_pCutscene && !_pImpl->_pCutscene->isElapsed();
}

} // namespace ng
