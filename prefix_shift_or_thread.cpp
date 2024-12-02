#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <fstream>
#include <algorithm>
#include "random_generator.cpp"

using namespace std;
//
// Constants
const int totalBits = 100;
atomic<int> totalMismatch(0); // Tracks total mismatches
atomic<int> completedTasks(0);
mutex outputMutex; // For synchronizing output
mutex queueMutex; // For synchronizing the task queue
condition_variable cv;
bool stop = false;
bool matched = false;

// Task structure for thread workers
struct Task {
    string sequence;
    int index;
};

// Task queue
queue<Task> taskQueue;

void saveToFile(const string& filePath, const string& content) {
    ofstream outputFile(filePath);
    if (!outputFile.is_open()) {
        cerr << "Error: Unable to open file for writing: " << filePath << endl;
        return;
    }
    outputFile << content;
    outputFile.close();
    cout << "Replaced text saved to file: " << filePath << endl;
}

vector<bitset<totalBits> > generateMasks(const string& text) {
    vector<bitset<totalBits> > masks(256); // 모든 ASCII 문자에 대한 마스크 생성

    // 초기화: 모든 비트를 1로 설정
    for (auto& mask : masks) {
        mask.set(); // 모든 비트를 1로 설정
    }

    // 각 문자 위치에 따라 마스크 업데이트
    for (size_t i = 0; i < text.size(); ++i) {
        char ch = toupper(text[i]);
        
        masks[ch].reset(i); // i번째 비트를 0으로 설정
        
    }

    return masks;
}

bool shiftOrWithMismatches(const string& text, const string& pattern,string& replacedText, int maxMismatches,int startpos) {
    int m = pattern.size();  // Pattern 길이
    vector<bitset<totalBits> > mask = generateMasks(pattern);
    vector<int> mismatchPositions; // Mismatch 위치 추적
    bitset<totalBits> state; // 상태 (초기: 모든 비트 1)
    state.set();

    int mismatches = 0; // Mismatch 카운트
    
    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];
        bitset<totalBits> prevState = state;

        // 현재 문자에 따라 상태 갱신
        state = (prevState << 1) | mask[ch];

        // 매칭 실패 처리: 현재 매칭 중인 글자 위치의 비트를 확인
        if (state[i]) { // 현재 매칭 위치의 비트가 0이면 실패
            mismatches++;
            mismatchPositions.push_back(i);
            cout << "mismatches at position " << i << endl;
            if (mismatches > maxMismatches) {
                cout << "Exceeded maximum mismatches at position " << i << endl;
                return false; // 최대 허용 Mismatch 초과
            }
            // 매칭 실패 시 상태를 초기화 (글자 수에 맞는 유효 상태로 설정)
            state.set();
            state.reset(i);
        }

        // 상태 출력
        cout << "Character: '" << ch << "' | State: " << state
            << " | Mismatches: " << mismatches << endl;

        // 매칭 조건 확인
        bitset<totalBits> matchBit;
        matchBit.set(m - 1); // m번째 비트를 1로 설정
        if ((state & matchBit) == 0) {
            lock_guard<mutex> lock(outputMutex);
            cout << mismatches << " mismatches." << endl;
            // Mismatch 위치를 기준으로 SNP 대체
            for (int pos : mismatchPositions) {
                cout << "mismatches at pos, replacedText[], pattern[] " << pos << ", "<<replacedText[pos+startpos]<< ", "<<pattern[pos]<< endl;
                replacedText[pos+startpos] = pattern[pos]; // 텍스트를 패턴의 문자로 대체
            }
            totalMismatch += mismatches;
            return true;
        }
    }

    cout << "No match found within " << maxMismatches << " mismatches." << endl;
    return false;
}


// Generate prefixes of a given size from the text
vector<string> generatePrefixes(const string& text, int prefixSize) {
    vector<string> prefixes;
    if (text.size() >= prefixSize) {
        prefixes.push_back(text.substr(0, prefixSize)); // 처음부터 prefixSize 길이만큼 자름
    }
    return prefixes;
}

// Prefix Comparison (1차 검증)
vector<int> prefixComparison(const string& text, const vector<string>& prefixes) {
    vector<int> matchPositions;
    size_t prefixSize = prefixes[0].size(); // prefix 크기 고정 (모든 prefix가 동일 길이라고 가정)
    
    for (size_t i = 0; i + prefixSize <= text.size(); ++i) { // 범위 체크
        //bool matched = false;
        string subText = text.substr(i, prefixSize);
        transform(subText.begin(), subText.end(), subText.begin(), ::toupper); // 대문자로 변환
        for (const auto& prefix : prefixes) {
            string upperPrefix = prefix;
            transform(upperPrefix.begin(), upperPrefix.end(), upperPrefix.begin(), ::toupper); // 대문자로 변환
            if (text.substr(i, prefixSize) == prefix) {
                matchPositions.push_back(i); // 시작 지점만 저장
                //matched = true; // 매칭된 상태 저장
                i += prefixSize - 1;        // 매칭된 위치 이후로 건너뜀
                break;
            }
        }
        // if (matched) {
        //     i += prefixSize - 1; // 매칭된 위치 이후로 건너뜀
        // }
    }
    return matchPositions;
}

// Worker thread function
void workerThread(const string& comparisonText, string& replacedText, int prefixSize, int maxMismatches, int totalTasks) {
    while (true) {
        Task task;
        {
            unique_lock<mutex> lock(queueMutex);
            cv.wait(lock, [] { return !taskQueue.empty() || stop; });

            if (stop && taskQueue.empty()) return;

            task = taskQueue.front();
            taskQueue.pop();
        }

        vector<string> prefixes = generatePrefixes(task.sequence, prefixSize);
        vector<int> startPositions = prefixComparison(comparisonText, prefixes);

        if (!startPositions.empty()) {
            for (int startPos : startPositions) {
                string subText = comparisonText.substr(startPos, task.sequence.size());
                if(shiftOrWithMismatches(subText, task.sequence, replacedText, maxMismatches, startPos))
                    matched = true; // 매칭 발생 기록
            }
            completedTasks++;
            double progress = (static_cast<double>(completedTasks) / totalTasks) * 100.0;
            cout << "\rProgress: " << fixed << setprecision(2) << progress << "% completed." << flush;
        }
    }
    
}
double calculateFinalErrorRate(const std::string& originalText, const std::string& transformedFileName) {
    // 변환된 파일 읽기
    std::ifstream inputFile(transformedFileName);
    if (!inputFile) {
        std::cerr << "파일을 열 수 없습니다: " << transformedFileName << std::endl;
        exit(1);
    }

    std::string transformedText((std::istreambuf_iterator<char>(inputFile)),
                                std::istreambuf_iterator<char>());

    // 두 문자열의 길이가 동일한지 확인
    if (originalText.length() != transformedText.length()) {
        std::cerr << "오류: 원본 문자열과 변환된 문자열의 길이가 다릅니다." << std::endl;
        exit(1);
    }

    // 오차 계산
    long long totalErrors = 0;
    long long totalLength = originalText.length();

    for (size_t i = 0; i < totalLength; ++i) {
        if (originalText[i] != transformedText[i]) {
            totalErrors++;
        }
    }

    // 오차율 계산
    double errorRate = (static_cast<double>(totalErrors) / totalLength) * 100.0;

    std::cout << "총 오차 개수: " << totalErrors << std::endl;
    std::cout << "총 문자열 길이: " << totalLength << std::endl;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "전체 문자열에 대한 오차율: " << errorRate << "%\n";

    return errorRate;
}
int main() {
    int n, m, prefixSize = 10, maxMismatches = 40;
    string comparisonText, replacedText;

    // Read comparison text
    ifstream inputFile("/Users/a1/algorithm/out_put_0.txt");
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to open file." << endl;
        return 1;
    }
    char ch;
    while (inputFile.get(ch)) {
        comparisonText += toupper(ch);
    }
    inputFile.close();
    replacedText = comparisonText;

    //Get DNA sequence details
    cout << "Enter the length of each DNA sequence (n): ";
    cin >> n;
    cout << "Enter the number of DNA sequences to generate (m): ";
    cin >> m;

    // Generate DNA sequences
    // vector<string> sequences = {
    //     "GAGATCGTCCCCAAAGGTAGTTCAAAGCACGGGCCC"
    // };
    vector<string> sequences = DnaGenerator(n, m);
    cout<<"complete generating"<<endl;
    int totalTasks = sequences.size();

    // Create threads
    int numThreads = thread::hardware_concurrency();
    vector<thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(workerThread, cref(comparisonText), ref(replacedText), prefixSize, maxMismatches,totalTasks);
    }

    // Add tasks to the queue
    {
        lock_guard<mutex> lock(queueMutex);
        for (size_t i = 0; i < sequences.size(); ++i) {
            taskQueue.push(Task{sequences[i], static_cast<int>(i + 1)});
        }
    }
    cv.notify_all();

    // Wait for threads to complete
    {
        lock_guard<mutex> lock(queueMutex);
        stop = true;
    }
    cv.notify_all();

    for (auto& thread : threads) {
        thread.join();
    }

    // Save results
    saveToFile("replaced_text.txt", replacedText);
    //double finalErrorRate = calculateFinalErrorRate(comparisonText, replacedText);
    // 주석 처리해도 안전하도록 수정
    // Ensure `calculateFinalErrorRate` is called after synchronization
    if (!comparisonText.empty() && !replacedText.empty()) {
        cout << "Comparing texts..." << endl;
        double finalErrorRate = calculateFinalErrorRate(comparisonText, "replaced_text.txt");
        cout << "Final error rate: " << finalErrorRate << "%\n";
    }

    if (matched) {
        cout << "SNP: " << totalMismatch
            << " SNP per total text: "
            << fixed << setprecision(6)
            << static_cast<long double>(totalMismatch) / comparisonText.size()
            << endl;
    } else {
        cout << "No matches found." << endl;
    }

    cout << "Total mismatches: " << totalMismatch << endl;
    return 0;
}
