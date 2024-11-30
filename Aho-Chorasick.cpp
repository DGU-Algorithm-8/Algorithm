#include <iostream>             // 표준 입출력 스트림 사용
#include <vector>               // 벡터 사용
#include <string>               // 문자열 사용
#include <fstream>              // 파일 입출력 사용
#include <iomanip>              // 진행률 출력에 필요한 헤더
#include <thread>               // C++11 스레드 라이브러리
#include <mutex>                // 뮤텍스 사용
#include <atomic>               // 원자적 변수 사용
#include <algorithm>            // remove_if 함수 사용
#include <cctype>               // isspace 함수 사용
#include <array>                // std::array 사용
#include <queue>                // 작업 큐를 위한 헤더 추가
#include <condition_variable>   // 조건 변수 사용
#include <functional>           // std::function 사용

#include "random_generator/DnaGenerator.h"       // DnaGenerator.h 헤더 파일 포함

// Constants
const unsigned int NUM_THREADS = std::thread::hardware_concurrency();

// 뮤텍스 및 조건 변수 선언
std::mutex output_mutex;
std::mutex queue_mutex;  // 작업 큐 동기화를 위한 뮤텍스

// 진행률 계산을 위한 원자적 변수
std::atomic<long long> total_processed(0);
long long total_work = 0; // 전체 작업량 (패턴 수 × 텍스트 길이)

// SNP 위치의 전체 집합을 저장하기 위한 전역 변수 추가
std::vector<bool> globalSnpPositions;

// 문자에서 인덱스로 변환
int charToIndex(char c) {
    switch (c) {
        case 'A': return 0;
        case 'T': return 1;
        case 'C': return 2;
        case 'G': return 3;
        default: return -1;
    }
}

// Aho-Corasick 트라이 노드 구조체
struct TrieNode {
    std::array<TrieNode*, 4> children; // A, T, C, G
    TrieNode* failure; // 실패 함수 포인터
    std::vector<int> output; // 매칭된 패턴의 인덱스

    TrieNode() : failure(nullptr) {
        children.fill(nullptr);
    }
};

// 트라이에 패턴 삽입
void insertPattern(TrieNode* root, const std::string& pattern, int patternIndex) {
    TrieNode* node = root;
    for (char c : pattern) {
        int idx = charToIndex(c);
        if (idx == -1) continue; // 유효하지 않은 문자 무시
        if (node->children[idx] == nullptr) {
            node->children[idx] = new TrieNode();
        }
        node = node->children[idx];
    }
    node->output.push_back(patternIndex);
}

// 실패 함수 계산
void buildFailureLinks(TrieNode* root) {
    std::queue<TrieNode*> q;
    root->failure = root;

    // 루트의 자식 노드의 실패 함수를 루트로 설정하고 큐에 삽입
    for (int i = 0; i < 4; ++i) {
        if (root->children[i]) {
            root->children[i]->failure = root;
            q.push(root->children[i]);
        }
    }

    // BFS를 사용하여 실패 함수 설정
    while (!q.empty()) {
        TrieNode* current = q.front();
        q.pop();

        for (int i = 0; i < 4; ++i) {
            TrieNode* child = current->children[i];
            if (!child) continue;

            // 실패 함수를 찾기 위해 부모의 실패 함수에서 동일한 문자를 찾음
            TrieNode* failure = current->failure;
            while (failure != root && failure->children[i] == nullptr) {
                failure = failure->failure;
            }
            if (failure->children[i] && failure->children[i] != child) {
                failure = failure->children[i];
            }

            child->failure = failure;

            // 출력 함수 병합
            for (int patternIndex : failure->output) {
                child->output.push_back(patternIndex);
            }

            q.push(child);
        }
    }
}

// Aho-Corasick 검색 함수 (오차 허용)
std::vector<long long> aho_corasick_search_approx(
    const std::string& text, TrieNode* root, const std::string& pattern, int d, long long start_pos, long long end_pos) {
    
    long long n = end_pos - start_pos;
    int m = pattern.length();

    // delta 테이블을 동적으로 생성하여 메모리 사용량 최적화
    // delta[s][c]는 상태 s에서 문자 c를 읽었을 때의 다음 상태
    int totalStates = (m + 1) * (d + 1);
    std::vector<std::array<int, 4>> delta(totalStates, std::array<int, 4>{-1, -1, -1, -1});

    // 상태 ID를 계산하는 함수
    auto stateID = [&](int i, int e) -> int {
        return i * (d + 1) + e;
    };

    // 상태에서 i와 e를 추출하는 함수
    auto getState = [&](int s, int& i, int& e) {
        e = s % (d + 1);
        i = s / (d + 1);
    };

    // delta 테이블 구축
    for (int s = 0; s < totalStates; ++s) {
        int i, e;
        getState(s, i, e);
        for (int c = 0; c < 4; ++c) {
            if (i == m) {
                delta[s][c] = s; // 수용 상태
            } else {
                char pc = pattern[i];
                int pc_index = charToIndex(pc);
                if (pc_index == c) {
                    delta[s][c] = stateID(i + 1, e);
                } else if (e < d) {
                    delta[s][c] = stateID(i + 1, e + 1);
                }
            }
        }
    }

    // 활성 상태를 비트셋으로 추적하여 성능 최적화
    std::vector<bool> activeStates(totalStates, false);
    activeStates[stateID(0, 0)] = true;

    std::vector<long long> matches;

    for (long long pos = 0; pos < n; ++pos) {
        int c = charToIndex(text[start_pos + pos]);
        if (c == -1) {
            // 유효하지 않은 문자면 루트 상태로 초기화
            activeStates.assign(totalStates, false);
            activeStates[stateID(0, 0)] = true;
            total_processed++;
            continue;
        }

        std::vector<bool> newActiveStates(totalStates, false);

        for (int s = 0; s < totalStates; ++s) {
            if (activeStates[s]) {
                int nextState = delta[s][c];
                if (nextState != -1) {
                    newActiveStates[nextState] = true;

                    int i, e;
                    getState(nextState, i, e);
                    if (i == m && e <= d) {
                        long long matchIndex = pos - m + 1;

                        // 매칭 조건 강화
                        if (matchIndex >= 0 && matchIndex + m <= n) {
                            int mismatchCount = 0;
                            bool validMatch = true;

                            for (int k = 0; k < m; ++k) {
                                if (text[start_pos + matchIndex + k] != pattern[k]) {
                                    mismatchCount++;
                                    if (mismatchCount > d) {
                                        validMatch = false;
                                        break;
                                    }
                                }
                            }

                            if (validMatch) {
                                long long actualMatchIndex = start_pos + matchIndex;
                                matches.push_back(actualMatchIndex);

                                // SNP 위치 기록
                                for (int k = 0; k < m; ++k) {
                                    if (text[start_pos + matchIndex + k] != pattern[k]) {
                                        long long snpPos = start_pos + matchIndex + k;
                                        if (snpPos >= 0 && snpPos < (long long)globalSnpPositions.size()) {
                                            globalSnpPositions[snpPos] = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 초기 상태 다시 활성화
        newActiveStates[stateID(0, 0)] = true;

        activeStates = std::move(newActiveStates);

        // 진행률 업데이트
        total_processed++;
    }

    return matches;
}

// 파일에서 서열 읽기 함수
std::string readSequenceFromFile(const std::string& fileName) {
    std::ifstream inputFile(fileName);
    if (!inputFile) {
        std::cerr << "파일을 열 수 없습니다: " << fileName << std::endl;
        exit(1);
    }

    std::string text;
    std::string line;

    // 파일 확장자 확인
    std::string extension;
    size_t dotPos = fileName.find_last_of(".");
    if (dotPos != std::string::npos) {
        extension = fileName.substr(dotPos + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }

    if (extension == "fa" || extension == "fasta") {
        // .fa 또는 .fasta 파일인 경우
        while (std::getline(inputFile, line)) {
            if (line.empty()) continue;
            if (line[0] == '>') {
                // 헤더 라인 무시
                continue;
            } else {
                // 공백 제거 및 서열 라인 연결
                line.erase(std::remove_if(line.begin(), line.end(),
                                          [](unsigned char c) { return std::isspace(c); }), line.end());
                text += line;
            }
        }
    } else {
        // 일반 텍스트 파일인 경우
        while (std::getline(inputFile, line)) {
            if (line.empty()) continue;
            // 공백 제거 및 라인 연결
            line.erase(std::remove_if(line.begin(), line.end(),
                                      [](unsigned char c) { return std::isspace(c); }), line.end());
            text += line;
        }
    }

    inputFile.close();

    if (text.empty()) {
        std::cerr << "서열 데이터가 없습니다." << std::endl;
        exit(1);
    }

    std::cout << "서열의 길이: " << text.length() << std::endl;
    return text;
}

int main() {
    std::string text;
    int patternLength;
    int d;
    int numPatterns;
    std::string textFileName;

    std::cout << "원본 문자열이 포함된 텍스트 파일의 이름을 입력하세요: ";
    std::cin >> textFileName;

    text = readSequenceFromFile(textFileName);

    std::cout << "랜덤 패턴의 길이를 입력하세요: ";
    std::cin >> patternLength;
    std::cout << "허용되는 오차 개수(d)를 입력하세요: ";
    std::cin >> d;
    std::cout << "생성할 랜덤 패턴의 개수: ";
    std::cin >> numPatterns;

    std::string matrixFile = "transition_matrix.txt";
    std::vector<std::vector<double>> transitionProb(4, std::vector<double>(4, 0.25)); // 기본 초기화

    std::ifstream checkFile(matrixFile);
    if (checkFile) {
        std::cout << "기존 전이 행렬을 '" << matrixFile << "'에서 로드합니다.\n";
        transitionProb = loadTransitionMatrix(matrixFile); // 실제 로딩 함수 필요
    } else {
        std::cerr << "오류: 전이 행렬을 찾을 수 없습니다. 제공하거나 생성해 주세요.\n";
        // 기본 전이 행렬 사용
    }

    // 패턴 생성
    std::vector<std::string> sequences = generateRandomDNASequences(transitionProb, patternLength, numPatterns);

    // Aho-Corasick 트라이 구축
    TrieNode* root = new TrieNode();
    for (int i = 0; i < sequences.size(); ++i) {
        insertPattern(root, sequences[i], i);
    }
    buildFailureLinks(root);

    // 전체 작업량 계산 (패턴 수 × 텍스트 길이)
    total_work = static_cast<long long>(numPatterns) * static_cast<long long>(text.length());

    // SNP 위치를 추적하기 위한 벡터 초기화
    globalSnpPositions.assign(text.length(), false);

    // 패턴별 매칭 결과 저장할 벡터 초기화
    std::vector<std::vector<long long>> allPatternMatches(numPatterns, std::vector<long long>());

    // 작업 큐: 각 작업은 (pattern_index, start_pos, end_pos)
    std::queue<std::tuple<int, long long, long long>> tasks;

    // 텍스트를 청크로 분할하고 작업 큐 초기화
    const int NUM_CHUNKS = 30;
    long long text_length = text.length();
    long long chunk_size = text_length / NUM_CHUNKS;
    long long overlap = patternLength - 1;

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        for (int p = 0; p < numPatterns; ++p) {
            for (int c = 0; c < NUM_CHUNKS; ++c) {
                long long start_pos = c * chunk_size;
                long long end_pos = (c == NUM_CHUNKS - 1) ? text_length : (start_pos + chunk_size + overlap);
                tasks.emplace(std::make_tuple(p, start_pos, end_pos));
            }
        }
    }

    // 스레드 풀 생성 및 작업 처리
    std::vector<std::thread> threads;

    // 스레드 함수 정의
    auto worker = [&](int thread_id) {
        while (true) {
            std::tuple<int, long long, long long> task;
            // 작업 큐에서 작업 가져오기
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (tasks.empty()) {
                    break;
                }
                task = tasks.front();
                tasks.pop();
            }

            int pattern_idx;
            long long start_pos, end_pos;
            std::tie(pattern_idx, start_pos, end_pos) = task;

            std::string pattern = sequences[pattern_idx];

            // 근사 매칭 수행
            std::vector<long long> matches = aho_corasick_search_approx(text, root, pattern, d, start_pos, end_pos);

            // 매칭 결과 저장
            if (!matches.empty()) {
                std::lock_guard<std::mutex> lock(output_mutex);
                allPatternMatches[pattern_idx].insert(allPatternMatches[pattern_idx].end(), matches.begin(), matches.end());
            }
        }
    };

    // 스레드 풀 생성
    for (unsigned int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(worker, t);
    }

    // 진행률 모니터링 스레드 추가
    std::thread progress_thread([&]() {
        while (total_processed < total_work) {
            double progress = (static_cast<double>(total_processed.load()) / total_work) * 100.0;
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "\r전체 진행률: " << std::fixed << std::setprecision(2) << progress << "% 완료";
                std::cout.flush();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 0.5초마다 갱신
        }
        // 진행률 100% 표시
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "\r전체 진행률: 100.00% 완료\n";
        }
    });

    // 모든 스레드가 작업을 완료할 때까지 대기
    for (auto& th : threads) {
        th.join();
    }

    // 진행률 스레드 종료 대기
    progress_thread.join();

    // 전체 SNP 개수는 중복되지 않은 SNP 위치의 개수
    long long totalSnps = 0;
    for (bool snp : globalSnpPositions) {
        if (snp) totalSnps++;
    }

    // 오차율 계산
    double snpPercentage = (static_cast<double>(totalSnps) / text.length()) * 100.0;

    // 패턴별 매칭 인덱스 출력
    for (int i = 0; i < numPatterns; ++i) {
        std::cout << "\n패턴 " << (i + 1) << ": " << sequences[i] << "\n매칭 인덱스: ";
        if (allPatternMatches[i].empty()) {
            std::cout << "없음";
        } else {
            for (size_t j = 0; j < allPatternMatches[i].size(); ++j) {
                std::cout << allPatternMatches[i][j] << (j < allPatternMatches[i].size() - 1 ? ", " : "");
            }
        }
        std::cout << "\n";
    }

    // 전체 SNP 개수 및 오차율 출력
    std::cout << "\n전체 SNP 개수: " << totalSnps << std::endl;
    std::cout << "전체 문자열 길이: " << text.length() << std::endl;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "전체 문자열에 대한 오차율: " << snpPercentage << "%\n";

    // 메모리 정리 (트라이 노드 삭제)
    std::function<void(TrieNode*)> deleteTrie = [&](TrieNode* node) {
        if (!node) return;
        for (auto child : node->children) {
            deleteTrie(child);
        }
        delete node;
    };

    deleteTrie(root);

    return 0;
}
// 컴파일 명령어:
// g++ -std=c++17 -pthread -O3 -o aho aho.cpp
