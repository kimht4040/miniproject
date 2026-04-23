#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <string>
#include <deque>
#include "bass.h"
#include "PlayGame.h"
#include "ShowMenu.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <SFML/Audio/Music.hpp>
#include <sqlite3.h>

#include "ShowResult.h"
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>

void setMacOSResourcePath() {
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) {
        chdir(path);
    }
    CFRelease(resourcesURL);
}
#endif

using namespace std;
namespace fs = std::filesystem;
enum class GameState {
    Login,
    Menu,
    PlayGame,
    Result,
    Defeat,
    Exit
};

struct MenuResult {
    GameState nextState = GameState::Menu;// 다음으로 갈 상태
    string id;
    fs::path selectedOsuPath = "";
    fs::path selectedMp3Path = "";
    string filename;
    int score = 0;
};
MenuResult result;
GameState StartLogin(sf::RenderWindow& window, sf::Font& font) {
    sf::String inputText = "";

    sf::Color bgColor(30, 34, 42);

    sf::Text titleText(font);
    titleText.setString("Enter Your ID");
    titleText.setCharacterSize(40);
    titleText.setFillColor(sf::Color(255, 255, 255, 230));
    titleText.setPosition({100.f, 150.f});


    sf::RectangleShape inputBox({400.f, 60.f});
    inputBox.setPosition({100.f, 220.f});
    inputBox.setFillColor(sf::Color(40, 44, 52));
    inputBox.setOutlineThickness(3.f);
    inputBox.setOutlineColor(sf::Color(97, 175, 239));


    sf::Text displayText(font);
    displayText.setCharacterSize(30); // 박스 안에 들어가도록 크기 축소
    displayText.setPosition({115.f, 230.f});


    sf::RectangleShape cursor({2.f, 35.f});
    cursor.setFillColor(sf::Color::White);
    sf::Clock cursorClock;
    bool isCursorVisible = true;

    while (window.isOpen()) {
        // --- [커서 깜빡임 로직 (0.5초 주기)] ---
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            isCursorVisible = !isCursorVisible;
            cursorClock.restart();
        }

        while (const auto event = window.pollEvent()) {

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Enter) {
                    // 아이디를 한 글자라도 입력했을 때만 넘어가도록 처리
                    if (!inputText.isEmpty()) {
                        result.id = inputText.toAnsiString();
                        result.nextState = GameState::Menu;
                        return GameState::Menu;
                    }
                }
            }

            if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                char32_t unicode = textEntered->unicode;

                // 백스페이스 처리
                if (unicode == 8) {
                    if (!inputText.isEmpty()) {
                        inputText.erase(inputText.getSize() - 1);
                    }
                }
                else if (unicode >= 32 && unicode != 127) {
                    inputText += unicode;
                }
            }
        }


        if (inputText.isEmpty()) {
            // 아무것도 입력하지 않았을 때의 Placeholder 텍스트
            displayText.setString("Type here...");
            displayText.setFillColor(sf::Color(150, 150, 150, 150)); // 반투명한 회색
            cursor.setPosition({115.f, 232.f}); // 커서는 텍스트 맨 앞에 위치
        } else {
            // 실제 입력 중인 텍스트
            displayText.setString(inputText);
            displayText.setFillColor(sf::Color::White);

            float textWidth = displayText.getGlobalBounds().size.x;
            cursor.setPosition({115.f + textWidth + 5.f, 232.f});
        }


        window.clear(bgColor);
        window.draw(titleText);
        window.draw(inputBox);
        window.draw(displayText);

        if (isCursorVisible) {
            window.draw(cursor);
        }

        window.display();
    }

    return GameState::Menu;
}
GameState ShowMenu(sf::RenderWindow& window, sf::Font& font) {
    fs::path project_path = "osu";
    vector<fs::path> mp3_path;
    vector<fs::path> osu_path;
    string target_mp3 = ".mp3";

    // 폴더 탐색 로직 (기존과 동일)
    try {
        if (fs::is_directory(project_path) && fs::exists(project_path)) {
            for (const auto& entry : fs::recursive_directory_iterator(project_path)) {
                if (fs::is_regular_file(entry)) {
                    if (entry.path().extension() == target_mp3) {
                        mp3_path.push_back(entry.path());
                    }
                }
            }
        } else {
            std::cerr << "경로가 존재하지 않거나 폴더가 아닙니다: " << project_path << '\n';
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "파일 시스템 에러 발생: " << e.what() << '\n';
    }

    // 메뉴 객체 초기화
    Menu_MP3 menu_mp3(MENU_X, MENU_Y, font, mp3_path);
    unique_ptr<Menu_SONG> menu_song = nullptr;
    bool is_song = false;
    sf::Music previewMusic;




    sf::Color bgColor(25, 28, 36);


    sf::Text headerText(font);
    headerText.setCharacterSize(45);
    headerText.setFillColor(sf::Color::White);
    headerText.setPosition({50.f, 40.f}); // 좌측 상단 여백


    sf::Text helpText(font);
    helpText.setCharacterSize(20);
    helpText.setFillColor(sf::Color(150, 150, 150, 200));
    helpText.setPosition({50.f, window.getSize().y - 60.f}); // 화면 맨 아래쪽


    sf::RectangleShape dimOverlay;
    dimOverlay.setSize(sf::Vector2f(window.getSize().x, window.getSize().y));
    dimOverlay.setFillColor(sf::Color(0, 0, 0, 170)); // 반투명 검은색

    // 메인 루프
    while (window.isOpen()) {

        // 상태에 따른 텍스트 업데이트
        if (is_song) {
            headerText.setString("SELECT DIFFICULTY");
            helpText.setString("Scroll: Navigate   |   Left Click: Select   |   ESC: Back to Songs");
        } else {
            headerText.setString("SELECT TRACK");
            helpText.setString("Scroll: Navigate   |   Left Click: Select");
        }

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    if (menu_song) {
                        is_song = false;
                        menu_song.reset(); // 메모리 해제
                        previewMusic.stop();
                    }
                }
            }

            if (const auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (scrollEvent->wheel == sf::Mouse::Wheel::Vertical) {
                    if (is_song && menu_song) {
                        menu_song->onScroll(scrollEvent->delta);
                    } else {
                        menu_mp3.onScroll(scrollEvent->delta);
                    }
                }
            }

            if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseEvent->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mouseCoords = window.mapPixelToCoords(mouseEvent->position);

                    if (is_song && menu_song) {
                        result.selectedOsuPath = menu_song->GetIndex(mouseCoords);
                        if (!(result.selectedOsuPath == "./no")) {
                            result.selectedMp3Path = get_mp3_from_osu(result.selectedOsuPath);
                            result.filename = result.selectedOsuPath.string();
                            result.nextState = GameState::PlayGame;
                            previewMusic.stop();
                            return GameState::PlayGame;
                        }
                    } else {
                        // SONG 메뉴가 꺼져 있을 때만 MP3 메뉴 클릭 감지
                        int selected_mp3 = menu_mp3.GetIndex(mouseCoords);
                        if (selected_mp3 != -1) {
                            menu_song = std::make_unique<Menu_SONG>(MENU_X, MENU_Y, font, mp3_path[selected_mp3]);
                            is_song = true;

                            if (previewMusic.openFromFile(mp3_path[selected_mp3].string())) {
                                previewMusic.setVolume(20.f);
                                previewMusic.setLooping(true);
                                previewMusic.setPlayingOffset(sf::seconds(10.f)); // 하이라이트 부분부터 재생되게 설정해둔 센스 아주 좋습니다!
                                previewMusic.play();
                            } else {
                                std::cout << "오디오 파일을 불러오지 못했습니다: " << mp3_path[selected_mp3] << std::endl;
                            }
                        }
                    }
                }
            }
        }

        // 업데이트 로직
        if (is_song) {
            menu_song->update(window);
        } else {
            menu_mp3.update(window);
        }


        window.clear(bgColor);


        window.draw(headerText);
        window.draw(helpText);

        menu_mp3.draw(window);


        if (is_song) {
            window.draw(dimOverlay);
            menu_song->draw(window);
        }

        window.display();
    }

    return GameState::PlayGame;
}
GameState PlayGame(sf::RenderWindow& window, const string &osuFile, const string mp3File, sf::Font &font, sf::Texture &texture, HSAMPLE hitSample) {
    // 파싱
    deque<Note> notes = parseOsuMania(osuFile);

    // 오디오 초기화
    HSTREAM stream = BASS_StreamCreateFile(FALSE, mp3File.c_str(), 0, 0, 0);
    if (stream == 0) {
        std::cerr << "오디오 파일 로드 실패!" << std::endl;
        BASS_Free();
    }



    Judgment judgment;

    vector<sf::Keyboard::Key> myKeys = {
        sf::Keyboard::Key::D, sf::Keyboard::Key::F, sf::Keyboard::Key::J, sf::Keyboard::Key::K
    };

    // 라인 긋기
    vector<sf::RectangleShape> lines;
    for (int i = 1; i <= 5; ++i) {
        sf::RectangleShape line(sf::Vector2f({2.f * SCALE_X, 1080.f * SCALE_Y}));
        line.setFillColor(sf::Color(100, 100, 100));
        line.setPosition(sf::Vector2f({(768.f + (i - 1) * 100.f) * SCALE_X-LINE_POSITION_X, 0.f}));
        lines.push_back(line);
    }

    sf::RectangleShape judgmentLine(sf::Vector2f({400.f * SCALE_X, 7.f * SCALE_Y}));
    judgmentLine.setFillColor(sf::Color(155,155,155,100));
    judgmentLine.setPosition(sf::Vector2f({768.f * SCALE_X-LINE_POSITION_X, JGLINE * SCALE_Y}));

    // 라인이 차지하는 공간 검정색 막 씌우기
    sf::RectangleShape lane_background(sf::Vector2f({400.f * SCALE_X, RESOLUTION_Y * SCALE_Y}));
    lane_background.setFillColor(sf::Color( 50, 50, 50, 200));
    lane_background.setPosition(sf::Vector2f({(768.f-LINE_POSITION_X) * SCALE_X, 0.f}));

    sf::Text comboText(font);
    comboText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
    comboText.setFillColor(sf::Color::Yellow);
    comboText.setPosition({(968.f-LINE_POSITION_X) * SCALE_X, 250.f * SCALE_Y});

    sf::Text judgeText(font);
    judgeText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
    judgeText.setFillColor(sf::Color::White);
    judgeText.setPosition({(968.f-LINE_POSITION_X) * SCALE_X, 450.f * SCALE_Y});

    sf::Text scoreText(font);
    scoreText.setCharacterSize(static_cast<unsigned int>(30 * SCALE_Y));
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition({(968.f-LINE_POSITION_X) * SCALE_X, 950.f * SCALE_Y});

    sf::RectangleShape hpGaugeFrame;
    hpGaugeFrame.setSize({26.f * SCALE_X, 360.f * SCALE_Y});
    hpGaugeFrame.setFillColor(sf::Color(40, 40, 40, 180));
    hpGaugeFrame.setOutlineThickness(3.f * SCALE_X);
    hpGaugeFrame.setOutlineColor(sf::Color(170, 170, 170));
    hpGaugeFrame.setPosition({730.f * SCALE_X - LINE_POSITION_X, 250.f * SCALE_Y});

    sf::RectangleShape hpGaugeFill;
    hpGaugeFill.setPosition({
        hpGaugeFrame.getPosition().x + 4.f * SCALE_X,
        hpGaugeFrame.getPosition().y + 4.f * SCALE_Y
    });

    sf::Text hpLabel(font);
    hpLabel.setString("HP");
    hpLabel.setCharacterSize(static_cast<unsigned int>(24 * SCALE_Y));
    hpLabel.setFillColor(sf::Color::White);
    sf::FloatRect hpLabelBounds = hpLabel.getLocalBounds();
    hpLabel.setOrigin(hpLabelBounds.position + hpLabelBounds.size / 2.f);
    hpLabel.setPosition({
        hpGaugeFrame.getPosition().x + hpGaugeFrame.getSize().x / 2.f,
        hpGaugeFrame.getPosition().y - 28.f * SCALE_Y
    });

    vector<sf::Text> keyGuideTexts;
    const vector<string> keyLabels = {"D", "F", "J", "K"};
    for (int i = 0; i < 4; ++i) {
        sf::Text keyText(font);
        keyText.setString(keyLabels[i]);
        keyText.setCharacterSize(static_cast<unsigned int>(42 * SCALE_Y));
        keyText.setFillColor(sf::Color(220, 220, 220));

        sf::FloatRect keyBounds = keyText.getLocalBounds();
        keyText.setOrigin(keyBounds.position + keyBounds.size / 2.f);

        const float laneCenterX = (819.f + static_cast<float>(i) * 100.f) * SCALE_X - LINE_POSITION_X;
        const float keyGuideY = (JGLINE - 40.f) * SCALE_Y;
        keyText.setPosition({laneCenterX, keyGuideY});
        keyGuideTexts.push_back(keyText);
    }

    // 배경화면 설정하기
    fs::path p(osuFile);
    fs::path dirPath = p.parent_path();// 검색해야할 폴더
    fs::path img_path;
    try {
        if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();

                    // JPG, JPEG, PNG 대소문자 모두 체크
                    if (ext == ".jpg" || ext == ".JPG" ||
                        ext == ".jpeg" || ext == ".JPEG" ||
                        ext == ".png" || ext == ".PNG") {

                        img_path = entry.path();
                        break; // 하나 찾으면 바로 종료
                        }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "에러: " << e.what() << std::endl;
    }
    // 찾아낸 이미지 파일 저장
    sf::Texture backgroundTexture; // 저장할 텍스쳐 객체
    if (!backgroundTexture.loadFromFile(img_path)) {
        std::cerr << "Failed to load background.png" << std::endl;
    }
    sf::Sprite backgroundSprite(backgroundTexture);
    sf::Vector2u textureSize = backgroundTexture.getSize();
    float scaleX = (static_cast<float>(RESOLUTION_X) / textureSize.x) * SCALE_X;
    float scaleY = (static_cast<float>(RESOLUTION_Y) / textureSize.y) * SCALE_Y;
    backgroundSprite.setScale({scaleX, scaleY});



    //노트 처리시 이펙트
    vector<Effect> effects;
    // 키입력시 누름 효과
    vector<Gear> gears;
    for (int i = 0; i < 4; ++i) {
        gears.emplace_back(i);
    }

    //라인별 노트 저장 큐
    deque<Note> laneNotes[4];
    for (auto& n : notes) {
        laneNotes[n.lane].push_back(n);
    }



    sf::Clock readyClock;
    bool isMusicPlaying = false;
    float delaySeconds = 2.0f;

    bool lanePressed[4] = {false, false, false, false};


    while (window.isOpen()) {
        float elapsed = readyClock.getElapsedTime().asSeconds();
        long long currentMusicTime = 0;

        if (!isMusicPlaying) {
            if (elapsed >= delaySeconds) {
                BASS_ChannelPlay(stream, FALSE);
                isMusicPlaying = true;
            }
            currentMusicTime = static_cast<long long>((elapsed - delaySeconds) * 1000.0);
        } else {

            QWORD bytePosition = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
            double seconds = BASS_ChannelBytes2Seconds(stream, bytePosition);
            currentMusicTime = static_cast<long long>(seconds * 1000.0);
        }
        if (judgment.getHp()<=0) {
            BASS_StreamFree(stream);
            result.nextState = GameState::Defeat;
            return GameState::Defeat;
        }
        for (int i = 0; i < 4; ++i) {
            while (!laneNotes[i].empty()) {
                Note& front = laneNotes[i].front();
                if (front.isProcessed) {
                    laneNotes[i].pop_front();
                    continue;
                }

                long long deadline = (front.isLongNote && front.isHolding) ? front.endTime : front.startTime;

                if (currentMusicTime - deadline > 300) {
                    judgment.BREAK();
                    judgeText.setString("MISS");
                    judgeText.setFillColor(sf::Color::Magenta);
                    sf::FloatRect bounds = judgeText.getLocalBounds();
                    judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                    judgeText.setScale({1.5f, 1.5f});
                    laneNotes[i].pop_front();
                } else {
                    break;
                }
            }
        }

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape ) {
                    BASS_StreamFree(stream);
                    result.nextState = GameState::Menu;
                    return GameState::Menu;
                }
                for (int i = 0; i < 4; ++i) {
                    if (keyPressed->code == myKeys[i]) {
                        lanePressed[i] = true;
                        HCHANNEL hitChannel = BASS_SampleGetChannel(hitSample, FALSE);
                        //BASS_ATTRIB_VOL 히트사운드 볼륨 조절임 뒤에 벨류 값으로
                        BASS_ChannelSetAttribute(hitChannel, BASS_ATTRIB_VOL, 1.f);
                        BASS_ChannelPlay(hitChannel, true);

                        if (!laneNotes[i].empty()) {
                            for (auto it = laneNotes[i].begin(); it != laneNotes[i].end(); ++it) {
                                Note& target = *it;
                                if (target.isProcessed || target.isHolding) {
                                    continue;
                                }

                                JudgeResult res = judgment.judge(currentMusicTime, target.startTime);
                                if (res != JudgeResult::None) {
                                    effects.emplace_back(texture, i);

                                    if (res == JudgeResult::Perfect) {
                                        judgeText.setString("PERFECT");
                                        judgeText.setFillColor(sf::Color::Cyan);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }else if (res == JudgeResult::Great) {
                                        judgeText.setString("GREAT");
                                        judgeText.setFillColor(sf::Color::Yellow);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }else if (res == JudgeResult::Good) {
                                        judgeText.setString("GOOD");
                                        judgeText.setFillColor(sf::Color::Green);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    } else if (res == JudgeResult::Miss) {
                                        judgeText.setString("MISS");
                                        judgeText.setFillColor(sf::Color::Magenta);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }

                                    if (res == JudgeResult::Miss) {
                                        //laneNotes[i].erase(it);
                                        target.isProcessed = true;
                                    } else if (target.isLongNote) {
                                        target.isHolding = true;
                                    } else {
                                        //laneNotes[i].erase(it);
                                        target.isProcessed = true;
                                    }

                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (const auto* KeyReleased = event->getIf<sf::Event::KeyReleased>()) {
                for (int i = 0; i < 4; ++i) {
                    if (KeyReleased->code == myKeys[i]) {
                        lanePressed[i] = false;
                        if (!laneNotes[i].empty()) {
                            Note& target = laneNotes[i].front();
                            if (target.isLongNote && target.isHolding) {
                                HCHANNEL hitChannel = BASS_SampleGetChannel(hitSample, FALSE);
                                BASS_ChannelPlay(hitChannel, true);
                                JudgeResult res = judgment.judge(currentMusicTime, target.endTime);

                                if (res == JudgeResult::None) {
                                    judgment.BREAK();
                                    judgeText.setString("MISS");
                                    judgeText.setFillColor(sf::Color::Magenta);
                                    sf::FloatRect bounds = judgeText.getLocalBounds();
                                    judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                    judgeText.setScale({1.5f, 1.5f});
                                } else {
                                    effects.emplace_back(texture, i);
                                    if (res == JudgeResult::Perfect) {
                                        judgeText.setString("PERFECT");
                                        judgeText.setFillColor(sf::Color::Cyan);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }else if (res == JudgeResult::Great) {
                                        judgeText.setString("GREAT");
                                        judgeText.setFillColor(sf::Color::Yellow);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }else if (res == JudgeResult::Good) {
                                        judgeText.setString("GOOD");
                                        judgeText.setFillColor(sf::Color::Green);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    } else if (res == JudgeResult::Miss) {
                                        judgeText.setString("MISS");
                                        judgeText.setFillColor(sf::Color::Magenta);
                                        sf::FloatRect bounds = judgeText.getLocalBounds();
                                        judgeText.setOrigin(bounds.position + bounds.size / 2.f);
                                        judgeText.setScale({1.5f, 1.5f});
                                    }
                                }
                                target.isProcessed = true;
                            }
                        }
                    }
                }
            }
        }
        // 기어효과 적용
        for (int i = 0; i < 4; ++i) {
            gears[i].update(lanePressed[i]);
        }

        //노트 이펙트 사라지는 속도 설정dt
        for (auto it = effects.begin(); it != effects.end(); ) {
            it->update(.03f);
            judgment.update(1.f,judgeText);
            if (it->isFinished()) {
                it = effects.erase(it);
            } else {
                ++it;
            }
        }
        if (judgeText.getScale().x > 1.0f) {
            // 현재 크기에 0.95를 곱해서 점점 작아지게 만듬
            judgeText.setScale(judgeText.getScale() * 0.95f);

            // 크기가 1.0배 이하로 떨어지면 딱 1.0으로 고정
            if (judgeText.getScale().x < 1.0f) {
                judgeText.setScale({1.0f, 1.0f});
            }
        }

        comboText.setString(to_string(judgment.getCombo()));
        scoreText.setString(to_string(judgment.getScore()));
        float hpRatio = static_cast<float>(judgment.getHp()) / 100.f;
        if (hpRatio < 0.f) hpRatio = 0.f;
        if (hpRatio > 1.f) hpRatio = 1.f;
        const float gaugeInnerWidth = hpGaugeFrame.getSize().x - 8.f * SCALE_X;
        const float gaugeInnerHeight = hpGaugeFrame.getSize().y - 8.f * SCALE_Y;
        hpGaugeFill.setSize({gaugeInnerWidth, gaugeInnerHeight * hpRatio});
        hpGaugeFill.setPosition({
            hpGaugeFrame.getPosition().x + 4.f * SCALE_X,
            hpGaugeFrame.getPosition().y + 4.f * SCALE_Y + gaugeInnerHeight - hpGaugeFill.getSize().y
        });
        if (judgment.getHp() <= 40) {
            hpGaugeFill.setFillColor(sf::Color::Red);
        }
        else if (judgment.getHp()<=70) {
            hpGaugeFill.setFillColor(sf::Color::Yellow);
        }
        else{
            hpGaugeFill.setFillColor(sf::Color::Green);
        }

        window.clear(sf::Color::Black);
        window.draw(backgroundSprite);
        window.draw(lane_background);

        for (int i = 0; i < 4; ++i) {
            gears[i].draw_beam(window);
        }

        for (const auto& line : lines) {
            window.draw(line);
        }

        window.draw(hpGaugeFrame);
        window.draw(hpGaugeFill);
        window.draw(hpLabel);

        for (auto& lane : laneNotes) {
            for (auto& note : lane) {
                if (note.isProcessed) continue;
                long long timeDiff = note.startTime - currentMusicTime;
                note.update(currentMusicTime, NOTE_SPEED, JGLINE);
                if (note.isLongNote) {
                    long long endDiff = note.endTime - currentMusicTime;
                    if (timeDiff <= 6000 && endDiff > -150) {
                        window.draw(note.shape);
                        window.draw(note.headshape);
                        window.draw(note.tailshape);
                    }
                } else {
                    if (timeDiff <= 1500 && timeDiff > -150) {
                        window.draw(note.shape);
                    }
                }
            }
        }

        if (judgeText.getString() != "") {
            sf::FloatRect jRect = judgeText.getLocalBounds();
            judgeText.setOrigin(jRect.getCenter());
            window.draw(judgeText);
        }

        //기어 배경 그리기
        gears[0].draw_gear_bg(window);
        if (judgment.getCombo() > 0) {
            sf::FloatRect CRect = comboText.getLocalBounds();
            comboText.setOrigin(CRect.getCenter());
            window.draw(comboText);
        }
        // 기어 그리기
        for (int i = 0; i < 4; ++i) {
            gears[i].draw_gear(window);
        }

        if (currentMusicTime < 3) {
            for (auto& keyText : keyGuideTexts) {
                window.draw(keyText);
            }
        }

        // 점수 텍스트 출력
        sf::FloatRect sRect = scoreText.getLocalBounds();
        scoreText.setOrigin(sRect.getCenter());
        window.draw(scoreText);

        // 이펙트 효과 그리기
        for (auto& effect : effects) {
            effect.draw(window);
        }

        window.draw(judgmentLine);
        window.display();

        if (isMusicPlaying && BASS_ChannelIsActive(stream) == BASS_ACTIVE_STOPPED) {
            break;
        }
    }


    BASS_StreamFree(stream);
    result.score = judgment.getScore();
    cout << result.score << endl;
    return GameState::Result;

};
GameState ShowResult(sf::RenderWindow& window, int score, sf::Font &font) {

    const std::string currentSongKey = result.selectedOsuPath.empty() ? result.filename : result.selectedOsuPath.string();
    const std::string currentSongName = result.selectedOsuPath.empty() ? fs::path(currentSongKey).filename().string() : result.selectedOsuPath.filename().string();

    DBResult dbResult = SaveScoreToDB(result.id, currentSongKey, score);
    std::vector<ScoreRecord> leaderboard = GetTopScores(currentSongKey);


    sf::Color bgColor(20, 22, 28);      // 깊은 다크 그레이
    sf::Color panelColor(35, 38, 48);   // 패널용 조금 더 밝은 색


    sf::RectangleShape scorePanel({600.f * SCALE_X, 400.f * SCALE_Y});
    scorePanel.setFillColor(panelColor);
    scorePanel.setOutlineThickness(2.f * SCALE_Y);
    scorePanel.setOutlineColor(sf::Color(100, 100, 100));
    scorePanel.setOrigin(scorePanel.getSize() / 2.f);
    scorePanel.setPosition({(RESOLUTION_X / 2.f) * SCALE_X, (RESOLUTION_Y / 2.f - 50.f) * SCALE_Y});


    sf::RectangleShape rankPanel({350.f * SCALE_X, 550.f * SCALE_Y});
    rankPanel.setFillColor(sf::Color(0, 0, 0, 120)); // 더 어두운 반투명
    rankPanel.setPosition({(RESOLUTION_X - 400.f) * SCALE_X, 80.f * SCALE_Y});

    sf::Text songTitleText(font, currentSongName, 30 * SCALE_Y);
    songTitleText.setFillColor(sf::Color(180, 220, 255));
    songTitleText.setPosition({50.f * SCALE_X, 40.f * SCALE_Y});

    sf::Text scoreLabel(font, "FINAL SCORE", 25 * SCALE_Y);
    scoreLabel.setFillColor(sf::Color(200, 200, 200));
    scoreLabel.setOrigin(scoreLabel.getLocalBounds().getCenter());
    scoreLabel.setPosition({scorePanel.getPosition().x, scorePanel.getPosition().y - 80.f * SCALE_Y});

    sf::Text scoreText(font, std::to_string(score), 100 * SCALE_Y);
    scoreText.setFillColor(sf::Color::Yellow);
    scoreText.setOrigin(scoreText.getLocalBounds().getCenter());
    scoreText.setPosition({scorePanel.getPosition().x, scorePanel.getPosition().y + 20.f * SCALE_Y});


    sf::Text popupText(font, "", 35 * SCALE_Y);
    if (dbResult.isNewUser) {
        popupText.setString("NEW PLAYER REGISTERED");
        popupText.setFillColor(sf::Color::Cyan);
    } else if (dbResult.isNewRecord) {
        popupText.setString("NEW RECORD!");
        popupText.setFillColor(sf::Color(255, 100, 100)); // 강렬한 레드/핑크
    }
    popupText.setOrigin(popupText.getLocalBounds().getCenter());
    popupText.setPosition({scorePanel.getPosition().x, scorePanel.getPosition().y + 140.f * SCALE_Y});


    sf::Text rankTitleText(font, "TOP 10", 30 * SCALE_Y);
    rankTitleText.setFillColor(sf::Color::White);
    rankTitleText.setPosition({rankPanel.getPosition().x + 20.f, rankPanel.getPosition().y + 20.f});

    std::string rankString = "";
    for (size_t i = 0; i < leaderboard.size(); ++i) {
        rankString += std::to_string(i + 1) + ". " + leaderboard[i].id + "\n   " + std::to_string(leaderboard[i].score) + "\n";
    }
    sf::Text rankListText(font, rankString.empty() ? "No Records" : rankString, 20 * SCALE_Y);
    rankListText.setLineSpacing(1.2f);
    rankListText.setPosition({rankPanel.getPosition().x + 25.f, rankPanel.getPosition().y + 80.f});


    sf::Text escText(font, "Press ESC to continue", 20 * SCALE_Y);
    escText.setFillColor(sf::Color(100, 100, 100));
    escText.setPosition({50.f * SCALE_X, (RESOLUTION_Y - 60.f) * SCALE_Y});


    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    result.filename = "";
                    result.selectedOsuPath = "";
                    result.selectedMp3Path = "";
                    return GameState::Menu;
                }
            }
        }

        window.clear(bgColor);

        window.draw(scorePanel);
        window.draw(rankPanel);
        window.draw(songTitleText);
        window.draw(scoreLabel);
        window.draw(scoreText);
        if (!popupText.getString().isEmpty()) window.draw(popupText);
        window.draw(rankTitleText);
        window.draw(rankListText);
        window.draw(escText);

        window.display();
    }
    return GameState::Menu;
}
// GameState ShowResult(sf::RenderWindow& window, int score, sf::Font &font) {
//     const std::string currentSongKey = result.selectedOsuPath.empty()
//         ? result.filename
//         : result.selectedOsuPath.string();
//     const std::string currentSongName = result.selectedOsuPath.empty()
//         ? fs::path(currentSongKey).filename().string()
//         : result.selectedOsuPath.filename().string();
//     result.filename = currentSongKey;
//
//     DBResult dbResult = SaveScoreToDB(result.id, currentSongKey, score);
//
//     std::vector<ScoreRecord> leaderboard = GetTopScores(currentSongKey);
//
//     sf::Text rankTitleText(font);
//     rankTitleText.setString("- TOP 10 -");
//     rankTitleText.setCharacterSize(static_cast<unsigned int>(40 * SCALE_Y));
//     rankTitleText.setFillColor(sf::Color::Yellow);
//     // 화면 우측 상단쯤으로 위치 잡기 (해상도에서 x값을 350정도 뺌)
//     rankTitleText.setPosition({(RESOLUTION_X - 350.f) * SCALE_X, 100.f * SCALE_Y});
//
//     sf::Text songTitleText(font);
//     songTitleText.setString(currentSongName);
//     songTitleText.setCharacterSize(static_cast<unsigned int>(24 * SCALE_Y));
//     songTitleText.setFillColor(sf::Color(180, 220, 255));
//     songTitleText.setPosition({(RESOLUTION_X - 350.f) * SCALE_X, 60.f * SCALE_Y});
//
//
//     sf::Text resultText(font);
//     resultText.setString("RESULT"); // 텍스트 내용 먼저 세팅
//     resultText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
//     resultText.setFillColor(sf::Color::White);
//     resultText.setPosition({RESOLUTION_X / 2.f * SCALE_X, 100.f * SCALE_Y});
//
//     sf::Text scoreText(font);
//     std::string textToDisplay = "score: " + std::to_string(score);
//     scoreText.setString(textToDisplay);
//     scoreText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
//     scoreText.setFillColor(sf::Color::White);
//     scoreText.setPosition({RESOLUTION_X / 2.f * SCALE_X, (resultText.getPosition().y + 200.f) * SCALE_Y});
//
//     //esc 텍스트
//     sf::Text escText(font);
//     escText.setString("press esc to open menu");
//     escText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
//     escText.setFillColor(sf::Color::White);
//     escText.setPosition({(RESOLUTION_X - 700.f) * SCALE_X, (RESOLUTION_Y - 300.f) * SCALE_Y});
//     sf::Text popupText(font);
//     popupText.setCharacterSize(static_cast<unsigned int>(45 * SCALE_Y));
//
//     // 조건에 따라 텍스트와 색상만 바꿔줍니다.
//     if (dbResult.isNewUser) {
//         popupText.setString("New ID Created!");
//         popupText.setFillColor(sf::Color::Cyan);
//     } else if (dbResult.isNewRecord) {
//         popupText.setString("NEW RECORD!");
//         popupText.setFillColor(sf::Color::Yellow);
//     }
//     std::string rankString = "";
//     for (size_t i = 0; i < leaderboard.size(); ++i) {
//         // "1. 홍길동 : 1500" 형태로 한 줄씩 추가하고 줄바꿈(\n)
//         rankString += std::to_string(i + 1) + ". " +
//                       leaderboard[i].id + " : " +
//                       std::to_string(leaderboard[i].score) + "\n";
//     }
//
//     // 만약 아무도 플레이한 기록이 없다면 (처음 플레이 시)
//     if (rankString.empty()) {
//         rankString = "No records yet.";
//     }
//
//     sf::Text rankListText(font);
//     rankListText.setString(rankString);
//     rankListText.setCharacterSize(static_cast<unsigned int>(30 * SCALE_Y));
//     rankListText.setFillColor(sf::Color::White);
//     // 줄바꿈이 있는 텍스트는 좌측 정렬이 이쁘므로 CenterTextOrigin을 쓰지 않습니다.
//     // 제목 바로 아래로 위치 지정
//     rankListText.setPosition({(RESOLUTION_X - 350.f) * SCALE_X, 160.f * SCALE_Y});
//
//
//
//     while (window.isOpen()) {
//         while (const std::optional event = window.pollEvent()) {
//             if (event->is<sf::Event::Closed>()) {
//                 window.close();
//             }
//             if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
//                 if (keyPressed->code == sf::Keyboard::Key::Escape ) {
//                     result.filename = "";
//                     result.selectedOsuPath = "";
//                     result.selectedMp3Path = "";
//                     result.nextState = GameState::Menu;
//                     return GameState::Menu;
//                 }
//             }
//         }
//
//         window.clear(sf::Color::Black);
//         if (resultText.getString() != "") {
//             sf::FloatRect jRect = resultText.getLocalBounds();
//             resultText.setOrigin(jRect.getCenter());
//             window.draw(resultText);
//         }
//         if (scoreText.getString() != "") {
//             sf::FloatRect jRect = scoreText.getLocalBounds();
//             scoreText.setOrigin(jRect.getCenter());
//             window.draw(scoreText);
//         }
//         if (popupText.getString() != "") {
//             window.draw(popupText);
//         }
//
//         window.draw(rankListText);
//         window.draw(rankTitleText);
//         window.draw(escText);
//         window.display();
//
//     }
//
//     result.filename = "";
//     result.selectedOsuPath = "";
//     result.selectedMp3Path = "";
//     return GameState::Menu;
// }
GameState ShowDefeat(sf::RenderWindow& window, sf::Font &font) {

    //defeat텍스트
    sf::Text DefeatText(font);
    DefeatText.setString("DEFEAT"); // 텍스트 내용 먼저 세팅
    DefeatText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
    DefeatText.setFillColor(sf::Color::Red);
    DefeatText.setPosition({(RESOLUTION_X / 2.f) * SCALE_X, (RESOLUTION_Y/2.f)* SCALE_Y});


    //esc 텍스트
    sf::Text escText(font);
    escText.setString("press esc to open menu");
    escText.setCharacterSize(static_cast<unsigned int>(50 * SCALE_Y));
    escText.setFillColor(sf::Color::White);
    escText.setPosition({(RESOLUTION_X - 700.f) * SCALE_X, (RESOLUTION_Y - 300.f) * SCALE_Y});
    sf::Text popupText(font);
    popupText.setCharacterSize(static_cast<unsigned int>(45 * SCALE_Y));

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape ) {
                    result.filename = "";
                    result.selectedOsuPath = "";
                    result.selectedMp3Path = "";
                    result.nextState = GameState::Menu;
                    return GameState::Menu;
                }
            }
        }


        window.clear(sf::Color::Black);

        window.draw(escText);
        if (DefeatText.getString() != "") {
            sf::FloatRect jRect = DefeatText.getLocalBounds();
            DefeatText.setOrigin(jRect.getCenter());
            window.draw(DefeatText);
        }
        window.display();
    }
}
int main() {
#ifdef __APPLE__
    setMacOSResourcePath(); // 맥일 때만 실행
#endif
    // db sqlite사용
    printf("%s\n", sqlite3_libversion());


    //윈도우 설정
    sf::RenderWindow window(sf::VideoMode({RESOLUTION_X, RESOLUTION_Y}), "Rhythm Cpp");
    window.setFramerateLimit(120);
    window.setKeyRepeatEnabled(false);
    BASS_SetConfig(BASS_CONFIG_BUFFER, 10);        // 버퍼를 10-40ms 사이로 줄임
    BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 5);   // 업데이트 주기를 5ms 정도로 설정
    // 베이스 초기화
    if (!BASS_Init(-1, 44100, 0, nullptr, nullptr)) {
        std::cerr << "BASS 초기화 실패 " << BASS_ErrorGetCode() << std::endl;
        return 1;
    }
    // 폰트 로드
    sf::Font font;
    if (!font.openFromFile("font/Anton.ttf")) {
        std::cout << "폰트 로드 실패" << std::endl;
    }
    //타격음 로드
    HSAMPLE hitSample = BASS_SampleLoad(FALSE,
        "output.wav",
        0, 0, 10, BASS_SAMPLE_OVER_POS );

    if (hitSample == 0) {
        std::cerr << "타격음 로드 실패 " << BASS_ErrorGetCode() << std::endl;
        return 1;
    }
    // 히트 이펙트 로드
    sf::Texture texture;
    if (!texture.loadFromFile("effect.png")) {
        cout << "이펙트 로드 실패" << endl;
        return 1;
    }

    GameState currentState = GameState::Login;

    while (window.isOpen() && currentState != GameState::Exit) {
        if (currentState == GameState::Login) {
            currentState = StartLogin(window, font);
        }
        else if (currentState == GameState::Menu) {
            currentState = ShowMenu(window, font);
        }
        else if (currentState == GameState::PlayGame) {
            currentState = PlayGame(window,
                result.selectedOsuPath.string(),
                result.selectedMp3Path.string(),
                font, texture, hitSample);
        }
        else if (currentState == GameState::Defeat) {
            currentState = ShowDefeat(window, font);
        }
        else if (currentState == GameState::Result) {
            currentState = ShowResult(window, result.score, font);
        }
    }


    BASS_SampleFree(hitSample);

    BASS_Free();
    return 0;
}
