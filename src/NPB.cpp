#include "NPB.hpp"
#include "CCMenuItemToggler.hpp"
#include "ButtonSprite.hpp"
#include "InputNode.hpp"
#include "proc.hpp"
#include <MinHook.h>
#include <process.h>
#include <thread>
#include "console.hpp"

#define CHILD_(x, y) static_cast<cocos2d::CCNode*>(x->getChildren()->objectAtIndex(y))
#define S_(x) std::to_string(x).c_str()
#define NOCLIP_TEXT_TAG 69420

template<typename T>
static T getChild(cocos2d::CCNode* x, int i) {
    return static_cast<T>(x->getChildren()->objectAtIndex(i));
}

uintptr_t readPointer(uintptr_t _p) {
    return *reinterpret_cast<uintptr_t*>(_p);
}

static constexpr const unsigned char noclipVisibleOpacity = 150;
static bool noclipToggled = false;
static constexpr const char* progressBarOptionKey   = "5507";
static constexpr const char* noclipTextOptionKey    = "5508";
static constexpr const char* ghostOptionKey         = "5509";

namespace GameManager {
	inline void* (__cdecl* getSharedState)();
	inline bool(__thiscall* getGameVariable)(void* self, const char* key);

	void load() {
        getSharedState = reinterpret_cast<decltype(getSharedState)>(base + 0xC4A50);
        getGameVariable = reinterpret_cast<decltype(getGameVariable)>(base + 0xC9D30);
    }
}

class PlayerObject : public cocos2d::CCSprite {
    public:
        static constexpr const int shadow = 105;
        inline static std::vector<cocos2d::CCSprite*> ghosts {};
        inline static bool alwaysShowGhosts = false;

        void onGhostsToggle(cocos2d::CCObject*) {
            alwaysShowGhosts = !alwaysShowGhosts;

            for (auto ghost : ghosts)
                ghost->setVisible(alwaysShowGhosts);
        }

        void __thiscall setOpacity(unsigned int _o) {
            reinterpret_cast<void(__thiscall*)(
                PlayerObject*, unsigned int
            )>(
                base + 0x1f7d40
            )(
                this, _o
            );
        }

        inline static void (__thiscall* death)(PlayerObject*, char);
        static void __fastcall deathHook(PlayerObject* _po, void*, char _ch) {
            death(_po, _ch);

            // check if player is in the editor
            if (
                readPointer(
                    readPointer(base + 0x3222D0) + 0x168
                )
            ) return;

            if (ghosts.size() > 0)
                if (ghosts.at(0) == nullptr)
                    ghosts.clear();
            
            for (auto spr : ghosts)
                spr->setVisible(true);

            if (!GameManager::getGameVariable(GameManager::getSharedState(), ghostOptionKey)) {
                auto outer = getChild<cocos2d::CCSprite*>(_po, 0);
                auto inner = getChild<cocos2d::CCSprite*>(outer, 0);

                auto ship = getChild<cocos2d::CCSprite*>(_po, 1);
                auto shipinner = getChild<cocos2d::CCSprite*>(ship, 0);

                auto obj = cocos2d::CCSprite::create();
                
                auto color1 = cocos2d::CCSprite::createWithTexture(outer->getTexture());
                color1->setTextureRect(outer->getTextureRect());
                color1->setTextureAtlas(outer->getTextureAtlas());
                color1->setRotation(outer->getRotation());
                color1->setScale(outer->getScale());
                color1->setColor(outer->getColor());
                color1->setOpacity(shadow);

                auto color2 = cocos2d::CCSprite::createWithTexture(inner->getTexture());
                color2->setTextureRect(inner->getTextureRect());
                color2->setTextureAtlas(inner->getTextureAtlas());
                color2->setRotation(inner->getRotation());
                color2->setScale(inner->getScale());
                color2->setColor(inner->getColor());
                color2->setOpacity(shadow);

                obj->addChild(color1);
                obj->addChild(color2);

                if (ship->isVisible()) {
                    auto shipc = cocos2d::CCSprite::createWithTexture(ship->getTexture());
                    shipc->setTextureRect(ship->getTextureRect());
                    shipc->setTextureAtlas(ship->getTextureAtlas());
                    shipc->setRotation(ship->getRotation());
                    shipc->setScale(ship->getScale());
                    shipc->setColor(ship->getColor());
                    shipc->setOpacity(shadow);

                    auto shipic = cocos2d::CCSprite::createWithTexture(shipinner->getTexture());
                    shipic->setTextureRect(shipinner->getTextureRect());
                    shipic->setTextureAtlas(shipinner->getTextureAtlas());
                    shipic->setRotation(shipinner->getRotation());
                    shipic->setScale(shipinner->getScale());
                    shipic->setColor(shipinner->getColor());
                    shipic->setOpacity(shadow);
                    
                    obj->addChild(shipc);
                    obj->addChild(shipic);
                }

                obj->setPosition(_po->getPosition());
                obj->setRotation(_po->getRotation());
                obj->setScale(_po->getScale());

                _po->getParent()->addChild(obj);

                ghosts.push_back(obj);
            }
        }

        static void loadHook() {
            MH_CreateHook(
                (PVOID)(base + 0x1efaa0),
                (LPVOID)PlayerObject::deathHook,
                (LPVOID*)&PlayerObject::death
            );
        }
};

namespace PlayLayer {
    unsigned int inputDelay = 0;

    struct pushButtonData {
        cocos2d::CCLayer* layer;
        int state;
        bool player;
    };

    inline bool (__thiscall* push_)(cocos2d::CCLayer*, int, bool);
    inline bool (__thiscall* release_)(cocos2d::CCLayer*, int, bool);

    DWORD WINAPI pushButton(void* _data) {
        std::this_thread::sleep_for(std::chrono::milliseconds(inputDelay));

        auto data = reinterpret_cast<pushButtonData*>(_data);

        push_(data->layer, data->state, data->player);

        delete data;

        return 0;
    }

    DWORD WINAPI releaseButton(void* _data) {
        std::this_thread::sleep_for(std::chrono::milliseconds(inputDelay));

        auto data = reinterpret_cast<pushButtonData*>(_data);

        release_(data->layer, data->state, data->player);

        delete data;

        return 0;
    }

    inline bool (__thiscall* init_)(cocos2d::CCLayer*, cocos2d::CCObject*);
    bool __fastcall initHook(cocos2d::CCLayer* _layer, void*, cocos2d::CCObject* _gamelevel_ig) {

        // clear ghosts so there aren't any
        // wacky invalid ccsprites crashing gd

        // also CCSprite::autorelease should
        // deal with freeing up memory for the pointers
        // right? so i just need to clear the
        // vector and not have to worry about
        // memory

        // if i'm correct this should fix all the
        // crashing issues tho

        PlayerObject::ghosts.clear();

        auto ret = init_(_layer, _gamelevel_ig);

        auto gmIns = GameManager::getSharedState();

        if (GameManager::getGameVariable(gmIns, progressBarOptionKey)) {
            if (_layer->getChildrenCount() > 9) {
                auto progressBar = (cocos2d::CCNode*)(_layer->getChildren()->objectAtIndex(8));
                auto percentage  = (cocos2d::CCNode*)(_layer->getChildren()->objectAtIndex(9));

                // setVisible doesn't work for some god knows reason so using this bodge instead
                progressBar->setPositionY(-100);

                auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

                percentage->setPositionX(winSize.width / 2);

                static_cast<cocos2d::CCLabelBMFont*>(percentage)->setAlignment(cocos2d::CCTextAlignment::kCCTextAlignmentCenter);
            }
        }

        if (!GameManager::getGameVariable(gmIns, noclipTextOptionKey)) {
            auto noclipText = cocos2d::CCLabelBMFont::create("Noclip enabled", "bigFont.fnt");
            
            noclipText->setScale(.5);
            noclipText->setTag(NOCLIP_TEXT_TAG);

            auto ntSize = noclipText->getScaledContentSize();

            noclipText->setPosition(20 + ntSize.width / 2, 20 + ntSize.height / 2);

            _layer->addChild(noclipText, 100);

            if (noclipToggled)
                noclipText->setOpacity(noclipVisibleOpacity);
            else
                noclipText->setOpacity(0);
        }
        
        return ret;
    }

    inline void (__fastcall* reset_)(cocos2d::CCLayer*);
    void __fastcall resetHook(cocos2d::CCLayer* _self) {
        reset_(_self);

        if (!PlayerObject::alwaysShowGhosts)
            if (PlayerObject::ghosts.size() > 0)
                for (auto ghost : PlayerObject::ghosts)
                    if (ghost != nullptr)
                        ghost->setVisible(false);
    }

    bool __fastcall pushHook(cocos2d::CCLayer* _self, uintptr_t, int _state, bool _player) {
        if (inputDelay)
            CreateThread(0, 0x1000, pushButton, new pushButtonData { _self, _state, _player }, 0, 0);
        else return push_(_self, _state, _player);
        
        return true;
    }

    bool __fastcall releaseHook(cocos2d::CCLayer* _self, uintptr_t, int _state, bool _player) {
        if (inputDelay)
            CreateThread(0, 0x1000, releaseButton, new pushButtonData { _self, _state, _player }, 0, 0);
        else return release_(_self, _state, _player);
        
        return true;
    }

    static MH_STATUS loadHook() {
        auto s = MH_CreateHook(
            (PVOID)(base + 0x1fb780),
            (LPVOID)PlayLayer::initHook,
            (LPVOID*)&PlayLayer::init_
        );

        MH_CreateHook(
            (PVOID)(base + 0x20bf00),
            (LPVOID)PlayLayer::resetHook,
            (LPVOID*)&PlayLayer::reset_
        );

        MH_CreateHook(
            (PVOID)(base + 0x111500),
            (LPVOID)PlayLayer::pushHook,
            (LPVOID*)&PlayLayer::push_
        );

        MH_CreateHook(
            (PVOID)(base + 0x111660),
            (LPVOID)PlayLayer::releaseHook,
            (LPVOID*)&PlayLayer::release_
        );

        return s;
    }
};

namespace MoreOptionsLayer {
    inline int(__thiscall* addToggle)(cocos2d::CCLayer* self, const char* display, const char* key, const char* extraInfo);
	inline bool(__thiscall* init)(cocos2d::CCLayer*);

	bool __fastcall initHook(cocos2d::CCLayer* self) {
        bool res = init(self);

        MoreOptionsLayer::addToggle(
            self,
            "Hide Progress Bar",
            progressBarOptionKey,
            nullptr
        );

        MoreOptionsLayer::addToggle(
            self,
            "Disable Noclip Text",
            noclipTextOptionKey,
            "Disables the text at the bottom-left of the screen when noclip is enabled."
        );

        MoreOptionsLayer::addToggle(
            self,
            "Disable Ghosts",
            ghostOptionKey,
            "When you die, a ghost of your icon is placed where you died."
        );

        /*

        auto input = InputNode::create(100.0f, "(ms)", "bigFont.fnt", "0123456789", 10);

        input->setPosition(50, 80);

        auto dict = reinterpret_cast<cocos2d::CCDictionary*>(
            reinterpret_cast<uintptr_t>(self) + 0x1E0
        );

        std::cout << dict << "\n";

        auto keys = dict->allKeys();

        std::cout << keys << "\n";

        for (unsigned int i = 0; i < keys->count(); i++)
            std::cout << reinterpret_cast<cocos2d::CCString*>(keys->objectAtIndex(i))->getCString() << "\n";

            //*/

        return res;
    }

	MH_STATUS loadHook() {
        auto m = MH_CreateHook(
            (PVOID)(base + 0x1DE8F0),
            MoreOptionsLayer::initHook,
            (PVOID*)&MoreOptionsLayer::init
        );
        
        MoreOptionsLayer::addToggle = reinterpret_cast<decltype(MoreOptionsLayer::addToggle)>(base + 0x1DF6B0);

        return m;
    };
}

namespace EditorPauseLayer {
	inline void(__fastcall* initMenu)(cocos2d::CCLayer*);

    void __fastcall initMenuHook(cocos2d::CCLayer* _layer) {
        initMenu(_layer);


    }

    MH_STATUS loadHook() {
        auto m = MH_CreateHook(
            (PVOID)(base + 0x73550),
            initMenuHook,
            (PVOID*)&initMenu
        );

        return m;
    }
}

inline bool writeMemory(
	uintptr_t const address,
	std::vector<uint8_t> const& bytes
) {
	return WriteProcessMemory(
		GetCurrentProcess(),
		reinterpret_cast<LPVOID>(base + address),
		bytes.data(),
		bytes.size(),
		nullptr
    );
}

class TICB : public gd::CCTextInputNode {
    public:
        void onApplyDelay(cocos2d::CCObject* pSender) {
            const char* t = this->m_pTextField->getString();

            if (sizeof t)
                PlayLayer::inputDelay = std::stoi(t);
        }
};

class PauseLayer {
    public:
        void onNoclipToggle(cocos2d::CCObject* pSender) {
            noclipToggled = !noclipToggled;

            auto scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
            auto layer = CHILD_(scene, 0);
            auto text = static_cast<cocos2d::CCLabelBMFont*>(layer->getChildByTag(NOCLIP_TEXT_TAG));

            if (noclipToggled) {
                writeMemory(0x20A23C, { 0xE9, 0x79, 0x06, 0x00, 0x00 });

                if (text != nullptr)
                    text->setOpacity(noclipVisibleOpacity);
            } else {
                writeMemory(0x20A23C, { 0x6A, 0x14, 0x8B, 0xCB, 0xFF });

                if (text != nullptr)
                    text->setOpacity(0);
            }
        }
        
        static inline void (__fastcall* init_)(cocos2d::CCNode*);
        static void __fastcall initHook(cocos2d::CCNode* _self) {
            init_(_self);

            //auto menu = (cocos2d::CCMenu*)(_self->getChildren()->objectAtIndex(6));

            auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

            auto m = cocos2d::CCMenu::create();

            auto test = CCMenuItemToggler::createWithText_mine(
                "Noclip",
                m,
                noclipToggled,
                (cocos2d::SEL_MenuHandler)&PauseLayer::onNoclipToggle,
                { 172, -163 }
            );

            test->setScale(.65f);

            m->addChild(test);


            auto ghosts = CCMenuItemToggler::createWithText_mine(
                "Ghosts",
                m,
                PlayerObject::alwaysShowGhosts,
                (cocos2d::SEL_MenuHandler)&PlayerObject::onGhostsToggle,
                { 172, -138 }
            );

            ghosts->setScale(.65f);

            m->setScale(.75f);

            m->addChild(ghosts);


            auto dInput = gd::CCTextInputNode::create(
                "Input delay (ms)",
                nullptr,
                "bigFont.fnt",
                100.0f,
                30.0f
            );

            dInput->setLabelPlaceholderColor({ 30, 60, 160 });
            dInput->setMaxLabelScale(.75f);
            dInput->setScale(.65f);
            dInput->setAllowedChars("0123456789");
            dInput->setPosition(winSize.width - 80.0f, winSize.height - 45.0f);

            _self->addChild(dInput);

            auto dSpr = ButtonSprite::create(
                "Apply", 0, 0, "bigFont.fnt", "GJ_button_01.png", 0, .8f
            );
            dSpr->setScale(.6f);

            auto dApply = CCMenuItemSpriteExtraGD::create(
                dSpr, dInput, (cocos2d::SEL_MenuHandler)&TICB::onApplyDelay
            );

            dApply->setPosition(winSize.width / 2 - 80.0f, winSize.height / 2 - 85.0f);

            m->addChild(dApply);


            _self->addChild(m);
        }

        static MH_STATUS loadHook() {
            return MH_CreateHook(
                (PVOID)(base + 0x1e4620),
                (LPVOID)initHook,
                (LPVOID*)&init_
            );
        }
};

bool NPB::createHook() {
    if (MH_Initialize() != MH_OK)
        return false;

    MH_STATUS stat;
    if ((stat = PlayLayer::loadHook()) != MH_OK)
        return false;

    if ((stat = PauseLayer::loadHook()) != MH_OK)
        return false;

    if ((stat = MoreOptionsLayer::loadHook()) != MH_OK)
        return false;

    #ifdef GDCONSOLE
    ModLdr::console::load();
    #endif

    auto hwnd = gd::getGDWindow();
    HANDLE hIcon = LoadImageA(0, "dumb.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    if (hIcon) {
        //Change both icons to the same icon handle.
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        //This will ensure that the application icon gets changed too.
        SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(GetWindow(hwnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    PlayerObject::loadHook();

    GameManager::load();

    return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}

void NPB::unload() {
    #ifdef GDCONSOLE
    ModLdr::console::unload();
    #endif

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

#ifdef GDCONSOLE
void NPB::awaitUnload() {
    std::string inp;
    getline(std::cin, inp);

    if (inp != "e")
        NPB::awaitUnload();
}
#endif

