#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <SFML/Audio.hpp>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

class Note { //입력 받은 노트를 화면에 띄어주는역할
    public:
    float key_time;
    int lane;
    sf::RectangleShape shape;
    Note(float key_time, int lane) : key_time(key_time), lane(lane) {
        shape.setSize(sf::Vector2f({98.f,10.f}));
        shape.setFillColor(sf::Color::Yellow);
        shape.setPosition({202.f + lane * 100.f, 0.f});
    }
    void update(float currentTime, float speed, float hitLineY) {
        float timeGap = key_time - currentTime;
        float y = hitLineY - (timeGap * speed);
        shape.setPosition({shape.getPosition().x, y});
    }

};

enum class JudgeResult { Perfect, Good, None, Miss };
class Judgment {// 판정 역할
    int score=0;
    int combo=0;
    public:
    JudgeResult judge(float key_time, float map_time ) {
        float diff = fabs(key_time - map_time);
        if (diff < 0.05) {
            score += 100; combo++;
            return JudgeResult::Perfect;
        }
        else if (diff < 0.2) {
            score += 50; combo++;
            return JudgeResult::Good;
        }
        return JudgeResult::None;
    }
    void addMiss() {
        combo = 0;
    }
    int getScore() const { return score; }
    int getCombo() const { return combo; }

};

vector<Note> loadNotes(const string&filename) { //텍스트 파일을 입력 받아서 노트 벡터 형태로 저장함
    vector<Note> notes;
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "파일을 열 수 없습니다." << endl;
        exit(1);
    }
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string timePart, lanePart;
        if (std::getline(ss, timePart, ',') && std::getline(ss, lanePart)) {
            float time = std::stof(timePart);
            int lane = std::stoi(lanePart);

            notes.emplace_back(time, lane);
        }
    }
    file.close();
    return notes;
}
int main() {

    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("/Users/kimht4040/Desktop/code/miniproject/project/output.wav")) {
        std::cout << "Could not load sound." << std::endl;
        return -1;
    }
    sf::Sound sound(buffer);






    // 인식할 키 입력 백터
    std::vector<sf::Keyboard::Key> myKeys = {
        sf::Keyboard::Key::S, sf::Keyboard::Key::D, sf::Keyboard::Key::K, sf::Keyboard::Key::L
    };

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Rhythm Cpp");
    window.setFramerateLimit(60);

    Judgment judgment; //판정 클래스

    // 4개의 건반 라인 구분선 그리기
    std::vector<sf::RectangleShape> lines;
    for (int i = 1; i <= 5; ++i) {
        sf::RectangleShape line(sf::Vector2f({2.f, 600.f})); // 두께 2, 길이 600
        line.setFillColor(sf::Color(100, 100, 100));
        line.setPosition(sf::Vector2f({200.f + (i - 1) * 100.f, 0.f})); // X좌표: 200, 300, 400, 500, 600
        lines.push_back(line);       // for문으로 라인 4개 만들어서 벡터에 삽입
    }
    // 하단 판정선 그리기
    sf::RectangleShape judgmentLine(sf::Vector2f({400.f, 5.f})); // 너비 400, 두께 5
    judgmentLine.setFillColor(sf::Color::Blue); //setFillColor : 색 뭘로 채울지 결정
    judgmentLine.setPosition(sf::Vector2f({200.f, 500.f}));      // X: 200, Y: 500 위치에 배치

    // 음악 재생 부분
    sf::Music music;
    if (!music.openFromFile("/Users/kimht4040/Desktop/code/miniproject/project/music.mp3")) {
        cout << "Could not load music." << endl;
        return 1;
    }
    music.play();
    music.setVolume(50.f);

     // 채보 불러오기
    std::vector<Note> gameNotes = loadNotes("/Users/kimht4040/Desktop/code/miniproject/project/cmake-build-debug/map.txt");
    std::vector<Note> laneNotes[4];
    for (auto& n : gameNotes) {
        laneNotes[n.lane].push_back(n);
    }

    // 폰트
    sf::Font font;
    if (!font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        std::cout << "폰트 로드 실패" << std::endl;
        return -1;
    }

    // 콤보 텍스트
    sf::Text comboText(font);
    comboText.setCharacterSize(40);
    comboText.setFillColor(sf::Color::Yellow);
    comboText.setPosition({400.f, 100.f});
    // 판정 텍스트
    sf::Text judgeText(font);
    judgeText.setCharacterSize(50);
    judgeText.setFillColor(sf::Color::White);
    judgeText.setPosition({400.f, 250.f});


    // 텍스트 잠시 후 사라지게
    sf::Clock uiClock;
    float showTime = 0.f; // 텍스트를 보여줄 남은 시간


    // 메인 루프
    while (window.isOpen()) {


        float currentTime = music.getPlayingOffset().asSeconds();// 현재 재생시간을 시간의 기준점으로 설정함

        comboText.setString(std::to_string(judgment.getCombo()));


        // 이벤트 처리
        while (const std::optional event = window.pollEvent()) {

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                for (int i = 0; i < 4; ++i) {
                    if (keyPressed->code == myKeys[i]) {
                        sound.play();
                        if (!laneNotes[i].empty()) {
                            // 가장 앞의 노트와 현재 시간 비교
                            Note& target = laneNotes[i].front();
                            JudgeResult res = judgment.judge(currentTime, target.key_time);

                            // 판정이 발생했다면 노트 제거
                            if (res != JudgeResult::None) {
                                laneNotes[i].erase(laneNotes[i].begin());
                            }
                            if (res == JudgeResult::Perfect) {
                                judgeText.setString("PERFECT");
                                judgeText.setFillColor(sf::Color::Cyan);
                                showTime = 0.5f; // 0.5초 동안 표시
                            } else if (res == JudgeResult::Good) {
                                judgeText.setString("GOOD");
                                judgeText.setFillColor(sf::Color::Green);
                            }
                        }
                    }
                }
            }

        }



        window.clear(sf::Color::Black);


        for (const auto& line : lines) {
            window.draw(line);// 라인이 벡터에 저장되어 있으니 그리는 경우에도 for문 사용해서 한줄씩 그림
        }
        for (int i = 0; i < 4; ++i) {
            for (auto it = laneNotes[i].begin(); it != laneNotes[i].end(); ) {
                it->update(currentTime, 500.f, 500.f); // 속도 400으로 조절

                // 판정선을 지나친 노트 처리 (Miss)
                if (currentTime > it->key_time + 0.15f) {
                    judgment.addMiss();
                    it = laneNotes[i].erase(it);
                } else {
                    // 화면에 보일 때만 그리기
                    if (it->shape.getPosition().y > -50.f && it->shape.getPosition().y < 600.f) {
                        window.draw(it->shape);
                    }
                    ++it;
                }
            }
        }
        if (judgeText.getString() != "") {
            // 중앙 정렬 (글자 바뀔 때마다 위치 고정)
            sf::FloatRect jRect = judgeText.getLocalBounds();
            judgeText.setOrigin(jRect.getCenter());
            window.draw(judgeText);
        }
        if (judgment.getCombo() > 0) { // 콤보가 1 이상일 때만 표시
            window.draw(comboText);
        }
        window.draw(judgeText);
        window.draw(judgmentLine);
        window.display();
    }

    return 0;
}