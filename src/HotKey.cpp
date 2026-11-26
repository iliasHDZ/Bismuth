#include "HotKey.hpp"

HotKey* HotKey::hotKeysHead = nullptr;
HotKey* HotKey::hotKeysTail = nullptr;

using namespace geode::prelude;

$on_mod(Loaded) {
    KeyboardInputEvent().listen([](KeyboardInputData& data) -> bool {
        bool success = false;
        if (data.action == KeyboardInputData::Action::Press) {
            HotKey* hotKey = HotKey::hotKeysHead;
            while (hotKey) {
                if (hotKey->key == data.key) {
                    hotKey->pressed();
                    success = true;
                }
                hotKey = hotKey->next;
            }
        }
        return false;
    }).leak();
}