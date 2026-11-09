#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <unordered_map>
#include <vector>

using namespace geode::prelude;

namespace {
    struct State {
        std::vector<GameObject*> rings;                // все орбы уровня
        std::unordered_map<GameObject*, int> windows;  // орб -> сколько тиков игрок уже в его хитбоксе
        int live = 0;                                  // текущее окно (пока игрок внутри орба)
        int last = 0;                                  // последнее завершённое окно
        CCLabelBMFont* label = nullptr;

        void reset() {
            rings.clear();
            windows.clear();
            live = 0;
            last = 0;
            label = nullptr;
        }

        void refreshLabel() {
            if (!label) return;
            label->setString(fmt::format("Window: {}  (last: {})", live, last).c_str());
        }
    } g;

    bool isRing(GameObject* o) {
        switch (o->m_objectType) {
            case GameObjectType::YellowJumpRing:
            case GameObjectType::PinkJumpRing:
            case GameObjectType::GravityRing:
            case GameObjectType::RedJumpRing:
            case GameObjectType::CustomRing:
            case GameObjectType::DashRing:
            case GameObjectType::GravityDashRing:
                return true;
            default:
                return false;
        }
    }
}

class $modify(CFBaseLayer, GJBaseGameLayer) {
    // вызывается на КАЖДЫЙ физический тик (а не на каждый кадр экрана)
    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);

        if (!g.label || !typeinfo_cast<PlayLayer*>(this)) return;

        PlayerObject* players[2] = { m_player1, m_player2 };
        bool changed = false;

        for (auto* ring : g.rings) {
            bool inside = false;
            bool near = false;

            for (auto* p : players) {
                if (!p) continue;
                if (std::abs(p->getPositionX() - ring->getPositionX()) < 150.f) near = true;
                else continue;
                if (p->getObjectRect().intersectsRect(ring->getObjectRect())) inside = true;
            }

            bool activated = ring->m_hasBeenActivated || ring->m_hasBeenActivatedP2;
            auto it = g.windows.find(ring);
            bool counting = it != g.windows.end();

            if (inside && (counting || !activated)) {
                int c = ++g.windows[ring];
                g.live = c;
                changed = true;
                if (activated) {
                    // клик произошёл на этом тике — окно закрыто
                    g.last = c;
                    g.windows.erase(ring);
                }
            } else if (counting) {
                // игрок вышел из орба, не нажав
                g.last = it->second;
                g.live = 0;
                g.windows.erase(it);
                changed = true;
            }
            (void)near;
        }

        if (changed) g.refreshLabel();
    }
};

class $modify(CFPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        g.reset();
        for (auto* obj : CCArrayExt<GameObject*>(m_objects)) {
            if (isRing(obj)) g.rings.push_back(obj);
        }

        auto win = CCDirector::get()->getWinSize();
        g.label = CCLabelBMFont::create("Window: 0  (last: 0)", "bigFont.fnt");
        g.label->setAnchorPoint({ 0.f, 1.f });
        g.label->setPosition({ 8.f, win.height - 8.f });
        g.label->setScale(0.45f);
        g.label->setOpacity(200);
        this->addChild(g.label, 1000);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        g.windows.clear();
        g.live = 0;
        g.refreshLabel();
    }

    void onQuit() {
        g.reset();
        PlayLayer::onQuit();
    }
};
