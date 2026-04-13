//
// Created by kimht4040 on 26. 4. 13..
//

#ifndef OSUPROJECT_RESULT_H
#define OSUPROJECT_RESULT_H
#include <string>
#include <vector>
using namespace std;
struct DBResult {
    bool isNewUser;
    bool isNewRecord;
};
struct ScoreRecord {
    std::string id;
    int score;
};
DBResult SaveScoreToDB(const std::string& userId, const std::string& filename, int score);
vector<ScoreRecord> GetTopScores(const std::string& songName);
#endif //OSUPROJECT_RESULT_H