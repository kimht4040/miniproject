#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <SFML/Audio.hpp>
using namespace std;
class notes {
    public:

};
int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Rhythm Cpp");
    window.setFramerateLimit(60);

    // 음악 재생 부분
    sf::Music music;
    if (!music.openFromFile("/Users/kimht4040/Desktop/code/miniproject/project/music.mp3")) {
        cout << "Could not load music." << endl;
        return 1;
    }
    music.play();
    music.setVolume(50.f);





    // 일단 기본 노트 구현// 노트 크기:98,10  노트 위치: 202, 302, 402 , 502
    sf::RectangleShape notes(sf::Vector2f({98.f,10.f}));
    notes.setFillColor(sf::Color::Yellow);
    notes.setPosition({302.f,0.f});


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

    // 2. 메인 게임 루프
    while (window.isOpen()) {

        float currentTime = music.getPlayingOffset().asSeconds();// 현재 재생시간을 시간의 기준점으로 설정함

        // 3. 이벤트 처리 (SFML 3.0 std::optional 방식)
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }




        // 4. 화면 업데이트
        window.clear(sf::Color::Black);
        // UI 요소 렌더링
        for (const auto& line : lines) {
            window.draw(line);// 라인이 벡터에 저장되어 있으니 그리는 경우에도 for문 사용해서 한줄씩 그림
        }
        window.draw(judgmentLine);
        window.draw(notes);
        window.display();
    }

    return 0;
}