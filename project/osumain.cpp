#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <SFML/Audio.hpp>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#define GET_LANE(x, keyCount) ((x) * (keyCount) / 512)


using namespace std;
struct Note {
    int lane;           // 0 ~ 3
    long long startTime; // ms
    long long endTime;   // ms (단타는 0 또는 startTime과 동일)
    bool isLongNote;
};
std::vector<Note> parseOsuMania(std::string filePath) {
    std::vector<Note> notes;
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
            Note note;

            int x = stoi(tokens[0]);
            note.lane = GET_LANE(x, 4);
            note.startTime = stol(tokens[2]);
            if (!(stoi(tokens[3]) & 128) ){
                note.isLongNote = false;
                note.endTime = 0;
            }
            else {
                note.isLongNote = true;
                size_t colonPos = tokens[5].find(':');
                if (colonPos != std::string::npos) {
                    note.endTime = std::stol(tokens[5].substr(0, colonPos));
                }
            }

            notes.push_back(note);
        }


    }


    file.close();
    return notes;
}

int main() {

    vector<Note> note = parseOsuMania("osu/173612"
                                      " xi - FREEDOM DiVE/xi - FREEDOM DiVE (razlteh) [4K Normal].osu");
    

    return 0;
}