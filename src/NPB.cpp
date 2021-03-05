#include "NPB.hpp"
#include "CCMenuItemToggler.hpp"
#include <cocos2d.h>
#include <MinHook.h>
#include <process.h>
#include "console.hpp"

#define CHILD_(x, y) static_cast<cocos2d::CCNode*>(x->getChildren()->objectAtIndex(y))
#define S_(x) std::to_string(x).c_str()
#define NOCLIP_TEXT_TAG 69420

uintptr_t base = (uintptr_t)GetModuleHandleA(0);

static constexpr const unsigned char noclipVisibleOpacity = 150;
static bool noclipToggled = false;
static constexpr const char* progressBarOptionKey   = "5507";
static constexpr const char* noclipTextOptionKey    = "5508";

namespace GameManager {
	inline void* (__cdecl* getSharedState)();
	inline bool(__thiscall* getGameVariable)(void* self, const char* key);

	void load() {
        getSharedState = reinterpret_cast<decltype(getSharedState)>(base + 0xC4A50);
        getGameVariable = reinterpret_cast<decltype(getGameVariable)>(base + 0xC9D30);
    }
}

namespace PlayLayer {
    cocos2d::CCLayer* layer = nullptr;

    inline void (__thiscall* init_)(cocos2d::CCLayer*, cocos2d::CCObject*);
    void __fastcall initHook(cocos2d::CCLayer* _layer, void*, cocos2d::CCObject* _gamelevel_ig) {
        init_(_layer, _gamelevel_ig);

        layer = _layer;

        auto gmIns = GameManager::getSharedState();

        if (GameManager::getGameVariable(gmIns, progressBarOptionKey)) {
            auto progressBar = (cocos2d::CCNode*)(_layer->getChildren()->objectAtIndex(8));
            auto percentage  = (cocos2d::CCNode*)(_layer->getChildren()->objectAtIndex(9));

            // setVisible doesn't work for some god knows reason so using this bodge instead
            progressBar->setPositionY(-100);

            auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

            percentage->setPositionX(winSize.width / 2);

            static_cast<cocos2d::CCLabelBMFont*>(percentage)->setAlignment(cocos2d::CCTextAlignment::kCCTextAlignmentCenter);
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
    }

    static MH_STATUS loadHook() {
        return MH_CreateHook(
            (PVOID)(base + 0x1fb780),
            (LPVOID)PlayLayer::initHook,
            (LPVOID*)&PlayLayer::init_
        );
    }
};

class PlayerObject : public cocos2d::CCSprite {
    public:
        inline static int test = 255;
        static constexpr const int shadow = 50;

        void __thiscall setOpacity(unsigned int _o) {
            reinterpret_cast<void(__thiscall*)(
                PlayerObject*, unsigned int
            )>(
                base + 0x1f7d40
            )(
                this, _o
            );
        }

        inline static cocos2d::CCObject* (__thiscall* objCopy)(cocos2d::CCObject* _obj);

        inline static void (__thiscall* opacity)(PlayerObject*, unsigned int);
        static void __fastcall opacityHook(PlayerObject* _p, void*, unsigned int _o) {
            if (_o == 0)
                opacity(_p, 0);
            else
                opacity(_p, test);

            //auto p = reinterpret_cast<PlayerObject*>(
            //    reinterpret_cast<uintptr_t>(_p) + 0xEC
            //);

            //p->setOpacity(test);
        }

        inline static void (__thiscall* death)(PlayerObject*, char);
        static void __fastcall deathHook(PlayerObject* _po, void*, char _ch) {
            death(_po, _ch);

            std::cout << "_po: " << _po << std::endl;

            auto obj = new cocos2d::CCObject();

            std::cout << "obj: " << obj << std::endl;

            auto pocopy = objCopy(obj);

            std::cout << "pocopy: " << pocopy << std::endl;

            //pocopy->setOpacity(shadow);

            //PlayLayer::layer->addChild(pocopy);
        }

        static void loadHook() {
            MH_CreateHook(
                (PVOID)(base + 0x1f7d40),
                (LPVOID)PlayerObject::opacityHook,
                (LPVOID*)&PlayerObject::opacity
            );

            MH_CreateHook(
                (PVOID)(base + 0x1efaa0),
                (LPVOID)PlayerObject::deathHook,
                (LPVOID*)&PlayerObject::death
            );

            auto h = GetModuleHandleA("libcocos2d.dll");

            std::cout << std::hex << "h: " << h << std::endl;

            auto addr = GetProcAddress(h, "?copy@CCObject@cocos2d@@QAEPAV12@XZ");

            std::cout << "addr: " << addr << std::endl;

            objCopy = reinterpret_cast<cocos2d::CCObject*(__thiscall*)(cocos2d::CCObject*)>(
                addr
            );
        }
};

namespace MoreOptionsLayer {
    inline int(__thiscall* addToggle)(void* self, const char* display, const char* key, const char* extraInfo);
	inline bool(__thiscall* init)(void*);

	bool __fastcall initHook(void* self) {
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

            auto menu = (cocos2d::CCMenu*)(_self->getChildren()->objectAtIndex(6));

            auto m = cocos2d::CCMenu::create();

            auto test = CCMenuItemToggler::createWithText_mine(
                "Noclip",
                m,
                noclipToggled,
                (cocos2d::SEL_MenuHandler)&PauseLayer::onNoclipToggle,
                { 175, -78 }
            );

            test->setScale(.65);

            m->addChild(test);

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

    ModLdr::console::load();
    
    PlayerObject::loadHook();

    GameManager::load();

    return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}

void NPB::unload() {
    ModLdr::console::unload();

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

void NPB::awaitUnload() {
    std::string inp;
    getline(std::cin, inp);

    if (inp.starts_with("."))
        PlayerObject::test = std::stoi(inp.substr(1));

    if (inp != "e")
        NPB::awaitUnload();
}

