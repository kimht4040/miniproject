#ifndef PLAYGAME_H
#define PLAYGAME_H

#include <SFML/Graphics.hpp>
#include <deque>
#include <string>

#define RESOLUTION_X 1920
#define RESOLUTION_Y 1080
#define SCALE_X (static_cast<float>(RESOLUTION_X) / 1920.f)
#define SCALE_Y (static_cast<float>(RESOLUTION_Y) / 1080.f)
#define JGLINE (700.f * SCALE_Y)
#define GET_LANE(x, keyCount) ((x) * (keyCount) / 512)
#define NOTE_SPEED (1.5f * SCALE_Y)
#define LINE_POSITION_X 600.f

class Effect {
private:
    sf::Sprite sprite;
    float lifetime;
    float maxLifetime;
    int lane;
public:
    Effect(const sf::Texture& texture, int lane);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    bool isFinished() const;
};


class Note {
public:
    int lane;
    long long startTime;
    long long endTime;
    bool isLongNote;
    bool isHolding;
    bool isProcessed;
    sf::RectangleShape shape;
    sf::RectangleShape headshape;
    sf::RectangleShape tailshape;

    Note(long long startTime = 0, long long endTime = 0, bool isLongNote = false, int lane = -1);
    void update(long long currentTime, float speed, float hitLineY);
};

enum class JudgeResult { Perfect,Great, Good, None, Miss };

class Judgment {
    int score = 0;
    int combo = 0;
    int hp = 100;

public:
    JudgeResult judge(long long key_time, long long map_time);
    void BREAK();
    int getScore() const;
    int getCombo() const;
    static void update(int dt, sf::Text& text);
    int getHp() const;
};

class Gear {
private:
    int lane;
    sf::VertexArray beam;
    sf::RectangleShape gear;
    sf::RectangleShape gear_bg;
    sf::Color baseColor;
    sf::Color beamColor;

public:

    Gear(int lane);

    void update(bool is_press);

    void draw_gear(sf::RenderWindow& window);

    void draw_beam(sf::RenderWindow& window);

    void draw_gear_bg(sf::RenderWindow& window);
};

std::deque<Note> parseOsuMania(std::string filePath);
#endif
