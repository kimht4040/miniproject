#include "PlayGame.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace std;
Gear::Gear(int lane) : lane(lane) {

    baseColor = sf::Color::Blue;

    // 기어 생성
    gear.setFillColor(baseColor);
    gear.setSize({90.f * SCALE_X, 50.f * SCALE_Y});



    float startX = 774.f * SCALE_X - LINE_POSITION_X;
    float laneWidth = 100.f * SCALE_X;
    float posX = startX + (static_cast<float>(lane) * laneWidth);



    gear.setPosition({posX, (JGLINE+ 50.f) * SCALE_Y });
    gear.setOutlineThickness(2.f);
    gear.setOutlineColor(sf::Color::Black);


    beamColor = sf::Color(130, 220, 255); // 연한 파랑
    beam.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
    beam.resize(4);

    float startX_b = 770.f * SCALE_X - LINE_POSITION_X;
    float laneWidth_b = 100.f * SCALE_X;
    float posX_b = startX_b + (static_cast<float>(lane) * laneWidth_b);

    float topY = 350.f; // 빔이 올라가는 높이
    float bottomY = (JGLINE + 48) * SCALE_Y;
    float rightX = posX_b + laneWidth_b;

    beam[0].position = {posX_b, topY};
    beam[1].position = {rightX, topY};
    beam[2].position = {posX_b, bottomY};
    beam[3].position = {rightX, bottomY};


    for (int i = 0; i < 4; ++i) {
        beam[i].color = sf::Color::Transparent;// 빔 초기 투명임
    }

    // 기어 배경 생성
    gear_bg.setFillColor(sf::Color(155,155,155));
    gear_bg.setSize({400.f * SCALE_X, (RESOLUTION_Y-gear.getPosition().y)* SCALE_Y});
    gear_bg.setPosition({startX_b,(JGLINE+ 50.f) * SCALE_Y });
}

void Gear::update(bool is_press) {
    if (is_press) {
        // 누를 때 기어 색상 변경 및 키빔 활성화
        gear.setFillColor(sf::Color(0,0,155));
        beam[0].color = sf::Color(beamColor.r, beamColor.g, beamColor.b, 0);
        beam[1].color = sf::Color(beamColor.r, beamColor.g, beamColor.b, 0);
        beam[2].color = sf::Color(beamColor.r, beamColor.g, beamColor.b, 230);
        beam[3].color = sf::Color(beamColor.r, beamColor.g, beamColor.b, 230);
    }
    else {
        // 뗄 때 원래 색상으로 복구 및 키빔 투명화
        gear.setFillColor(baseColor);
        for (int i = 0; i < 4; ++i) {
            beam[i].color = sf::Color::Transparent;
        }
    }
}

void Gear::draw_gear(sf::RenderWindow& window) {
    window.draw(gear);
}
void Gear::draw_gear_bg(sf::RenderWindow& window) {
    window.draw(gear_bg);
}
void Gear::draw_beam(sf::RenderWindow& window) {
    window.draw(beam);
}

Effect::Effect(const sf::Texture& texture, int lane) : lifetime(.5f), maxLifetime(.5f), sprite(texture), lane(lane) {
    sf::FloatRect bounds = sprite.getLocalBounds();
    sprite.setOrigin(bounds.position + bounds.size / 2.f);
    sprite.setPosition({(100.f * static_cast<float>(lane) + 733.f)-(LINE_POSITION_X-90) * SCALE_X, JGLINE-10 * SCALE_Y});
    sprite.setScale({1.f * SCALE_X, 1.f * SCALE_Y});
}

void Effect::update(float dt) {
    lifetime -= dt;
    float alpha = (lifetime / maxLifetime) * 255;
    float beta = lifetime;
    sprite.setColor(sf::Color(255, 255, 255, static_cast<unsigned char>(alpha)));
    sprite.setScale({lifetime * SCALE_X + .7f, lifetime * SCALE_Y+ .7f});
}

void Effect::draw(sf::RenderWindow& window) {
    window.draw(sprite);
}

bool Effect::isFinished() const { return lifetime <= 0.f; }





Note::Note(long long startTime, long long endTime, bool isLongNote, int lane)
    : startTime(startTime), endTime(endTime), isLongNote(isLongNote), lane(lane), isHolding(false), isProcessed(false) {
    sf::Color noteColor = (lane == 0 || lane == 3) ? sf::Color::White : sf::Color::Yellow;
    float startX = (770.f + static_cast<float>(lane) * 100.f) * SCALE_X-LINE_POSITION_X;
    if (isLongNote) {
        // 롱노트 설정
        headshape.setFillColor(noteColor);
        headshape.setSize({98.f * SCALE_X, 15.f * SCALE_Y});
        headshape.setPosition({startX, 0.f});

        tailshape.setFillColor(noteColor);
        tailshape.setSize({98.f * SCALE_X, 15.f * SCALE_Y});
        tailshape.setPosition({startX, 0.f});


        shape.setFillColor(sf::Color(noteColor.r, noteColor.g, noteColor.b, 150)); // 몸통은 약간 투명하게
        shape.setSize({80.f * SCALE_X, static_cast<float>(endTime - startTime) * NOTE_SPEED});

        shape.setPosition({startX + 9.f * SCALE_X, 0.f});
    } else {
        shape.setFillColor(noteColor);
        shape.setSize({98.f * SCALE_X, 15.f * SCALE_Y});
        shape.setPosition({startX, 0.f});
    }
}

void Note::update(long long currentTime, float speed, float hitLineY) {
    float timeGap = static_cast<float>(startTime - currentTime);
    float y = hitLineY - (timeGap * speed);

    if (isLongNote) {
        headshape.setPosition({headshape.getPosition().x, y - headshape.getSize().y});
        float endTimeGap = static_cast<float>(endTime - currentTime);
        float tailY = hitLineY - (endTimeGap * speed);
        tailshape.setPosition({tailshape.getPosition().x, tailY - tailshape.getSize().y});
        shape.setPosition({shape.getPosition().x, tailY});
    } else {
        shape.setPosition({shape.getPosition().x, y - shape.getSize().y});
    }
}

JudgeResult Judgment::judge(long long key_time, long long map_time) {
    long long diff = std::abs(key_time - map_time);
    if (diff < 70) {
        score += 100; combo++;
        if (hp<=98) {
            hp+=2;
        }
        return JudgeResult::Perfect;
    }
    else if (diff < 120 ) {
        score += 70; combo++;
        if (hp<=98) {
            hp+=2;
        }
        return JudgeResult::Great;
    }
    else if (diff < 170) {
        score += 50; combo++;
        if (hp<=98) {
            hp+=2;
        }
        return JudgeResult::Good;
    }
    else if (diff < 220) {
        BREAK();
        return JudgeResult::Miss;
    }
    return JudgeResult::None;
}
void Judgment::update(int dt, sf::Text& text) {

}

void Judgment::BREAK() { combo = 0; hp -= 10;}

int Judgment::getScore() const { return score; }

int Judgment::getCombo() const { return combo; }

int Judgment::getHp() const { return hp; }


std::deque<Note> parseOsuMania(std::string filePath) {
    std::deque<Note> notes;
    std::string line;
    bool isHitObjectSection = false;
    fstream file;
    file.open(filePath, ios::in);
    if (!file.is_open()) {
        std::cerr << "파일을 열 수 없습니다: " << filePath << std::endl;
        return notes;
    }
    while (getline(file, line)) {
        if (line.find("[HitObjects]") != string::npos) {
            isHitObjectSection = true;
            continue;
        }
        if (isHitObjectSection && !line.empty()) {
            stringstream ss(line);
            string s;
            vector<string> tokens;
            while (getline(ss, s, ',')) {
                tokens.push_back(s);
            }
            if (tokens.size() < 5) continue;

            int x = stoi(tokens[0]);
            long long st = stol(tokens[2]);
            bool isLong = (stoi(tokens[3]) & 128) != 0;
            long long et = 0;
            if (isLong) {
                size_t colonPos = tokens[5].find(':');
                if (colonPos != std::string::npos) {
                    et = std::stol(tokens[5].substr(0, colonPos));
                }
            }
            int laneIdx = GET_LANE(x, 4);

            Note note(st, et, isLong, laneIdx);
            notes.push_back(note);
        }
    }
    file.close();
    return notes;
}