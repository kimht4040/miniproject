#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <fstream>
#include <vector>
#include <optional>
#include <iostream>

struct NoteData {
    float time;
    int lane;
};

int main() {
    sf::RenderWindow window(sf::VideoMode({400, 300}), "Chart Recorder");
    sf::Music music;

    // 경로가 길어 가독성을 위해 별도 변수로 뺍니다.
    std::string musicPath = "/Users/kimht4040/Desktop/code/miniproject/project/music.mp3";
    if (!music.openFromFile(musicPath)) {
        std::cout << "음악 파일을 찾을 수 없습니다!" << std::endl;
        return -1;
    }

    std::vector<sf::Keyboard::Key> keys = {
        sf::Keyboard::Key::S, sf::Keyboard::Key::D, sf::Keyboard::Key::K, sf::Keyboard::Key::L
    };

    std::vector<NoteData> recordedNotes;
    bool isStarted = false;

    std::cout << "SPACE: 녹화 시작 / 녹화 중 SPACE: 저장 후 종료" << std::endl;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {

                // 스페이스바 처리 로직 변경
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    if (!isStarted) {
                        // 1. 시작 전이면 녹화 시작
                        isStarted = true;
                        music.play();
                        std::cout << "녹화 시작!" << std::endl;
                    } else {
                        // 2. 녹화 중이면 창을 닫아 루프를 탈출 (자동으로 아래 저장 로직으로 이동)
                        std::cout << "녹화 중단 및 저장 시퀀스 시작..." << std::endl;
                        window.close();
                    }
                }

                // 노트 기록 로직
                if (isStarted) {
                    for (int i = 0; i < 4; ++i) {
                        if (keyPressed->code == keys[i]) {
                            float currentTime = music.getPlayingOffset().asSeconds();
                            recordedNotes.push_back({currentTime, i});
                            std::cout << "기록됨: " << currentTime << "s, 라인: " << i << std::endl;
                        }
                    }
                }
            }
        }

        window.clear(sf::Color::Black);
        window.display();

        // 음악이 끝나도 자동으로 닫히도록 유지
        if (isStarted && music.getStatus() == sf::SoundStream::Status::Stopped) {
            window.close();
        }
    }

    // [중요] while 루프가 끝나면(창이 닫히면) 실행되는 구간입니다.
    if (!recordedNotes.empty()) {
        std::ofstream outFile("map.txt");
        for (const auto& note : recordedNotes) {
            outFile << note.time << "," << note.lane << "\n";
        }
        outFile.close();
        std::cout << "저장 완료! 총 " << recordedNotes.size() << "개의 노트를 기록했습니다." << std::endl;
    }

    return 0;
}