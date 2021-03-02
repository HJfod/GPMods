#include <cocos2d.h>

class CCMenuItemToggler : public cocos2d::CCMenuItem {
    public:
        static CCMenuItemToggler* create(
            cocos2d::CCSprite* _spr_off,
            cocos2d::CCSprite* _spr_on,
            cocos2d::CCMenu* _parent,
            cocos2d::SEL_MenuHandler _click
        ) {
            auto ret = reinterpret_cast<CCMenuItemToggler*(__fastcall*)(
                cocos2d::CCSprite*,
                cocos2d::CCSprite*,
                cocos2d::CCMenu*,
                cocos2d::SEL_MenuHandler
            )>(
                (uintptr_t)GetModuleHandleA(0) + 0x19600
            )(
                _spr_off,
                _spr_on,
                _parent,
                _click
            );

            __asm { add esp, 0x8 }

            return ret;
        }

        void toggle(bool _toggle) {
            reinterpret_cast<void(__thiscall*)(
                CCMenuItemToggler*, bool
            )>(
                (uintptr_t)GetModuleHandleA(0) + 0x199b0
            )(
                // for some god knows reason true = not toggled
                this, !_toggle
            );
        }

        static CCMenuItemToggler* createWithText(
            cocos2d::SEL_MenuHandler _click,
            bool _is_toggled,
            cocos2d::CCMenu* _parent,
            cocos2d::CCArray* _wtf,
            const char* _text
        ) {
            return reinterpret_cast<CCMenuItemToggler*(__stdcall*)(
                cocos2d::SEL_MenuHandler,
                bool,
                cocos2d::CCMenu*,
                cocos2d::CCArray*,
                const char*
            )>(
                (uintptr_t)GetModuleHandleA(0) + 0x25cf00
            )(
                _click,
                _is_toggled,
                _parent,
                _wtf,
                _text
            );
        }

        static CCMenuItemToggler* createWithText_mine(
            const char* _text,
            cocos2d::CCMenu* _parent,
            bool _is_toggled,
            cocos2d::SEL_MenuHandler _click,
            cocos2d::CCPoint _pos
        ) {
            auto toggle = CCMenuItemToggler::create(
                cocos2d::CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png"),
                cocos2d::CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png"),
                _parent,
                _click
            );

            toggle->setPosition(_pos);

            toggle->toggle(_is_toggled);

            auto text = cocos2d::CCLabelBMFont::create(_text, "bigFont.fnt");

            text->setAlignment(cocos2d::CCTextAlignment::kCCTextAlignmentLeft);

            text->setPositionY(_pos.y); 

            text->setScale(.5);

            auto twidth = text->getScaledContentSize().width;

            text->setPositionX(_pos.x + twidth / 2 + 20);

            _parent->addChild(text);

            return toggle;
        }
};