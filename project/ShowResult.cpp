//
// Created by kimht4040 on 26. 4. 13..
//

#include "ShowResult.h"

#include <iostream>
#include <sqlite3.h>
#include <SFML/Graphics/Rect.hpp>


DBResult SaveScoreToDB(const std::string& userId, const std::string& filename, int score) {
    sqlite3* db;
    DBResult finalResult = {false, false};

    if (sqlite3_open("game_data.db", &db)) {
        std::cerr << "DB Error: " << sqlite3_errmsg(db) << std::endl;
        return finalResult;
    }

    const char* createTableSQL =
        "CREATE TABLE IF NOT EXISTS USERS (ID TEXT PRIMARY KEY);"
        "CREATE TABLE IF NOT EXISTS SCORES (ID TEXT, SONG TEXT, SCORE INT);";
    sqlite3_exec(db, createTableSQL, 0, 0, 0);

    sqlite3_stmt* stmt;

    // --- 1단계: 유저가 존재하는지 확인 ---
    const char* checkUserSQL = "SELECT COUNT(*) FROM USERS WHERE ID = ?;";
    sqlite3_prepare_v2(db, checkUserSQL, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);

    bool userExists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0) {
        userExists = true;
    }
    sqlite3_finalize(stmt);

    // --- 2단계: 유저 존재 여부에 따른 분기 처리 ---
    if (!userExists) {
        // [경우 1] 처음 접속한 아이디
        finalResult.isNewUser = true;

        // USERS 테이블에 아이디 추가
        const char* insertUserSQL = "INSERT INTO USERS (ID) VALUES (?);";
        sqlite3_prepare_v2(db, insertUserSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // SCORES 테이블에 기록 삽입
        const char* insertScoreSQL = "INSERT INTO SCORES (ID, SONG, SCORE) VALUES (?, ?, ?);";
        sqlite3_prepare_v2(db, insertScoreSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, filename.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, score);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

    } else {
        // [경우 2, 3, 4] 이미 존재하는 아이디 -> '같은 곡(filename)' 기준으로 검사
        const char* checkScoreSQL = "SELECT SCORE FROM SCORES WHERE ID = ? AND SONG = ?;";
        sqlite3_prepare_v2(db, checkScoreSQL, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, filename.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // 이 곡을 플레이한 적이 있음
            int existingScore = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);

            if (score > existingScore) {
                // [경우 3] 최고점 갱신 성공! (UPDATE)
                const char* updateScoreSQL = "UPDATE SCORES SET SCORE = ? WHERE ID = ? AND SONG = ?;";
                sqlite3_prepare_v2(db, updateScoreSQL, -1, &stmt, nullptr);
                sqlite3_bind_int(stmt, 1, score);
                sqlite3_bind_text(stmt, 2, userId.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 3, filename.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                finalResult.isNewRecord = true;
            }

        } else {
            sqlite3_finalize(stmt);

            const char* insertScoreSQL = "INSERT INTO SCORES (ID, SONG, SCORE) VALUES (?, ?, ?);";
            sqlite3_prepare_v2(db, insertScoreSQL, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, filename.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, score);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            finalResult.isNewRecord = true; // 첫 기록이니까 신기록 팝업!
        }
    }

    sqlite3_close(db);
    return finalResult;
}

vector<ScoreRecord> GetTopScores(const std::string& songName) {

    std::vector<ScoreRecord> topScores;
    sqlite3* db;

    if (sqlite3_open("game_data.db", &db)) {
        return topScores; // 열기 실패하면 빈 리스트 반환
    }

    const char* sql = "SELECT ID, SCORE FROM SCORES WHERE SONG = ? ORDER BY SCORE DESC LIMIT 10;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, songName.c_str(), -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ScoreRecord record;
            record.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            record.score = sqlite3_column_int(stmt, 1);
            topScores.push_back(record);
        }
    }

    // 메모리 해제 및 DB 닫기
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return topScores;
}