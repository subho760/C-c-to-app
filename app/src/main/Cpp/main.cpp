#include <jni.h>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

enum Direction { NONE = 0, UP = 1, RIGHT = 2, DOWN = 3, LEFT = 4 };

struct Point { int x, y; };

class GameEngine {
public:
    int size;
    int level;
    int lives;
    std::vector<int> board;
    std::mt19937 rng;

    GameEngine() {
        rng.seed(std::time(0));
        init(1);
    }

    void init(int lvl) {
        level = lvl;
        lives = 3;
        if (level <= 5) size = 4;
        else if (level <= 15) size = 5;
        else if (level <= 30) size = 6;
        else size = 8;
        generateSolvableBoard();
    }

    bool isPathClear(int x, int y, int dir) {
        int dx = 0, dy = 0;
        if (dir == UP) dy = -1;
        else if (dir == RIGHT) dx = 1;
        else if (dir == DOWN) dy = 1;
        else if (dir == LEFT) dx = -1;

        int cx = x + dx;
        int cy = y + dy;
        while (cx >= 0 && cx < size && cy >= 0 && cy < size) {
            if (board[cy * size + cx] != NONE) return false;
            cx += dx;
            cy += dy;
        }
        return true;
    }

    bool tap(int x, int y) {
        if (x < 0 || x >= size || y < 0 || y >= size) return false;
        int idx = y * size + x;
        int dir = board[idx];
        if (dir == NONE) return false;

        if (isPathClear(x, y, dir)) {
            board[idx] = NONE;
            return true;
        } else {
            lives--;
            return false;
        }
    }

    void generateSolvableBoard() {
        board.assign(size * size, NONE);
        int arrowCount = (size * size * (std::min(40 + level, 80))) / 100;
        
        // Use Reverse Solving to guarantee solvability
        std::vector<Point> coords;
        for(int i=0; i<size; ++i) for(int j=0; j<size; ++j) coords.push_back({i, j});
        
        std::vector<Point> placed;
        int attempts = 0;
        while(placed.size() < arrowCount && attempts < 500) {
            int rx = rng() % size;
            int ry = rng() % size;
            if(board[ry * size + rx] != NONE) { attempts++; continue; }
            
            int dirs[] = {UP, RIGHT, DOWN, LEFT};
            std::shuffle(std::begin(dirs), std::end(dirs), rng);
            
            bool ok = false;
            for(int d : dirs) {
                if(isPathClear(rx, ry, d)) {
                    board[ry * size + rx] = d;
                    if(canSolve(board)) {
                        placed.push_back({rx, ry});
                        ok = true;
                        break;
                    } else {
                        board[ry * size + rx] = NONE;
                    }
                }
            }
            attempts++;
        }
        
        if (placed.empty()) generateSolvableBoard(); // Fallback
    }

    bool canSolve(std::vector<int> tempBoard) {
        bool changed = true;
        while(changed) {
            changed = false;
            for(int y=0; y<size; ++y) {
                for(int x=0; x<size; ++x) {
                    int idx = y * size + x;
                    int dir = tempBoard[idx];
                    if(dir == NONE) continue;
                    
                    // Internal check
                    int dx=0, dy=0;
                    if (dir == UP) dy = -1; else if (dir == RIGHT) dx = 1;
                    else if (dir == DOWN) dy = 1; else if (dir == LEFT) dx = -1;
                    
                    bool blocked = false;
                    int cx = x + dx, cy = y + dy;
                    while(cx >= 0 && cx < size && cy >= 0 && cy < size) {
                        if(tempBoard[cy * size + cx] != NONE) { blocked = true; break; }
                        cx += dx; cy += dy;
                    }
                    
                    if(!blocked) {
                        tempBoard[idx] = NONE;
                        changed = true;
                    }
                }
            }
        }
        for(int val : tempBoard) if(val != NONE) return false;
        return true;
    }

    Point findHint() {
        for(int y=0; y<size; ++y) {
            for(int x=0; x<size; ++x) {
                int dir = board[y * size + x];
                if(dir != NONE && isPathClear(x, y, dir)) return {x, y};
            }
        }
        return {-1, -1};
    }
};

static GameEngine engine;

extern "C" {

JNIEXPORT void JNICALL Java_com_night_backgroundchange_MainActivity_initGame(JNIEnv* env, jobject obj) {
    engine = GameEngine();
}

JNIEXPORT void JNICALL Java_com_night_backgroundchange_MainActivity_generateLevel(JNIEnv* env, jobject obj, jint level) {
    engine.init(level);
}

JNIEXPORT jboolean JNICALL Java_com_night_backgroundchange_MainActivity_tapArrow(JNIEnv* env, jobject obj, jint x, jint y) {
    return engine.tap(x, y);
}

JNIEXPORT jintArray JNICALL Java_com_night_backgroundchange_MainActivity_getBoardState(JNIEnv* env, jobject obj) {
    jintArray result = env->NewIntArray(engine.board.size());
    env->SetIntArrayRegion(result, 0, engine.board.size(), engine.board.data());
    return result;
}

JNIEXPORT jint JNICALL Java_com_night_backgroundchange_MainActivity_getBoardSize(JNIEnv* env, jobject obj) {
    return engine.size;
}

JNIEXPORT jboolean JNICALL Java_com_night_backgroundchange_MainActivity_isLevelComplete(JNIEnv* env, jobject obj) {
    for(int v : engine.board) if(v != NONE) return false;
    return true;
}

JNIEXPORT jint JNICALL Java_com_night_backgroundchange_MainActivity_getLives(JNIEnv* env, jobject obj) {
    return engine.lives;
}

JNIEXPORT jint JNICALL Java_com_night_backgroundchange_MainActivity_getCurrentLevel(JNIEnv* env, jobject obj) {
    return engine.level;
}

JNIEXPORT jintArray JNICALL Java_com_night_backgroundchange_MainActivity_getHint(JNIEnv* env, jobject obj) {
    Point p = engine.findHint();
    jintArray result = env->NewIntArray(2);
    int arr[2] = {p.x, p.y};
    env->SetIntArrayRegion(result, 0, 2, arr);
    return result;
}

}
