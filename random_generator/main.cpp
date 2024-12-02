/*
프로그램 설명:
이 프로그램은 DNA 문자열을 입력받아 De Bruijn Graph를 구축하고, 
주어진 패턴과 허용 오차를 기반으로 SNP(Single Nucleotide Polymorphism)을 탐지합니다.

주요 기능:
1. De Bruijn Graph 구축
   - DNA 문자열의 k-mer를 이용하여 그래프를 생성합니다.
2. 패턴 매칭
   - 주어진 패턴이 DNA 문자열의 어느 위치에서 몇 개의 오차로 매칭되는지 탐지합니다.
3. 병렬 처리
   - 입력된 DNA 문자열을 여러 청크로 나누어 병렬로 처리하여 성능을 향상시킵니다.
4. 결과 출력
   - 각 패턴에 대해 매칭된 결과를 출력하고, 총 SNP 개수, 문자열 길이, 오차율 등을 계산합니다.

입력 파일:
- DNA 서열 파일
- 탐지 패턴의 길이, 허용 오차, 생성할 랜덤 패턴의 개수

출력:
- 패턴 매칭 결과 (인덱스와 오차)
- 전체 SNP 개수, 문자열 길이, 오차율

제작자: 팀 프로젝트
*/

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <mutex>
#include <thread>

using namespace std;

// De Bruijn 그래프를 관리하는 클래스
// 이 클래스는 DNA 문자열의 k-mer를 기반으로 그래프를 생성하고
// 패턴 매칭 기능을 제공합니다.
class DeBruijnGraph {
public:
    unordered_map<string, vector<string>> adjList; // 그래프의 인접 리스트 표현
    int k; // k-mer의 크기

    // De Bruijn 그래프 생성
    // DNA 문자열에서 k-1 접두사와 접미사를 추출하여 그래프를 구성합니다.
    void buildGraph(const string& dna, int kMer) {
        k = kMer;
        adjList.clear();
        for (size_t i = 0; i <= dna.size() - k; ++i) {
            string prefix = dna.substr(i, k - 1);
            string suffix = dna.substr(i + 1, k - 1);
            adjList[prefix].push_back(suffix);
        }
    }

    // 주어진 패턴과 DNA 문자열 간의 매칭을 수행
    // 허용 오차 이내에서 매칭된 패턴의 위치를 반환합니다.
    vector<pair<int, int>> matchPattern(const string& dna, const string& pattern, int allowedErrors) {
        vector<pair<int, int>> matches;
        size_t patternLen = pattern.size();
        for (size_t i = 0; i <= dna.size() - patternLen; ++i) {
            string subSeq = dna.substr(i, patternLen);
            int errors = countErrors(subSeq, pattern);
            if (errors <= allowedErrors) {
                matches.emplace_back(i, errors);
            }
        }
        return matches;
    }

private:
    // 두 문자열 간의 오차(일치하지 않는 문자 수)를 계산
    int countErrors(const string& seq1, const string& seq2) {
        int errors = 0;
        for (size_t i = 0; i < seq1.size(); ++i) {
            if (seq1[i] != seq2[i]) errors++;
        }
        return errors;
    }
};

// 전역 변수와 mutex를 활용하여 병렬 처리 시 동기화 문제를 해결
mutex mtx;

// 특정 DNA 청크를 처리하는 함수
// 이 함수는 De Bruijn Graph를 생성하고 패턴 매칭을 수행합니다.
void processChunk(const string& dna, int k, const vector<string>& patterns, int allowedErrors, int& totalSNPs, int threadId) {
    DeBruijnGraph graph;
    graph.buildGraph(dna, k);

    for (const string& pattern : patterns) {
        vector<pair<int, int>> matches = graph.matchPattern(dna, pattern, allowedErrors);
        
        lock_guard<mutex> lock(mtx); // 동기화를 위해 mutex 사용
        for (const auto& match : matches) {
            totalSNPs++;
            cout << "패턴: " << pattern << ", 인덱스: " << match.first << ", 오차: " << match.second << endl;
        }
    }
}

// DNA 서열 파일을 읽어 문자열로 반환하는 함수
string readDNAFile(const string& dnaFile) {
    ifstream file(dnaFile);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << dnaFile << endl;
        exit(1);
    }
    string dnaSequence, line;
    while (getline(file, line)) {
        dnaSequence += line;
    }
    file.close();
    return dnaSequence;
}

// 프로그램 진입점
// 사용자 입력을 받아 프로그램을 실행합니다.
int main() {
    string dnaFile;
    cout << "DNA 서열 파일의 경로를 입력하세요: ";
    cin >> dnaFile;

    // DNA 파일 읽기
    string dnaSequence = readDNAFile(dnaFile);

    int k, patternLength, allowedErrors, numPatterns;
    cout << "k-mer 크기를 입력하세요: ";
    cin >> k;
    cout << "탐지 패턴의 길이를 입력하세요: ";
    cin >> patternLength;
    cout << "허용되는 오차 개수를 입력하세요: ";
    cin >> allowedErrors;
    cout << "생성할 랜덤 패턴의 개수: ";
    cin >> numPatterns;

    // 랜덤 패턴 생성
    vector<string> patterns;
    for (int i = 0; i < numPatterns; ++i) {
        patterns.push_back(dnaSequence.substr(rand() % (dnaSequence.size() - patternLength + 1), patternLength));
    }

    // 병렬 처리를 위한 설정
    int numThreads = 4; // 스레드 수는 시스템에 따라 조정 가능
    vector<thread> threads;
    int totalSNPs = 0;

    // DNA 문자열을 청크로 나누어 처리
    size_t chunkSize = dnaSequence.size() / numThreads;
    for (int i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i == numThreads - 1) ? dnaSequence.size() : start + chunkSize;

        threads.emplace_back(processChunk, dnaSequence.substr(start, end - start), k, patterns, allowedErrors, ref(totalSNPs), i);
    }

    // 모든 스레드가 작업을 완료할 때까지 대기
    for (auto& t : threads) {
        t.join();
    }

    // 최종 결과 출력
    cout << "\n총 SNP 개수: " << totalSNPs << endl;
    cout << "전체 문자열 길이: " << dnaSequence.size() << endl;
    cout << "전체 문자열에 대한 오차율: " << fixed << setprecision(6) << (static_cast<double>(totalSNPs) / dnaSequence.size()) * 100 << "%" << endl;

    return 0;
}
