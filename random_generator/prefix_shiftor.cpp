#include <iostream>
#include <string>
#include <vector>
#include <bitset> // For binary representation
#include <math.h>
#include <fstream>

using namespace std;
const int totalBits = 250;
//const double initialmask = pow(2, totalBits);
// Function to generate masks for each character in the input text
// vector<unsigned long long> generateMasks(const string& text) {
//     int textLength = text.size();
//     vector<unsigned long long> masks(256, ~0ULL); // Initialize masks for all characters with all bits set to 1

//     // For each character in the text, update its mask
//     for (int i = 0; i < textLength; ++i) {
//         char ch = text[i];
//         masks[ch] &= ~(1ULL << i); // Set the bit corresponding to the character's index to 0
//     }
    

//     return masks;
// }
vector<bitset<totalBits> > generateMasks(const string& text) {
    vector<bitset<totalBits> > masks(256); // 모든 ASCII 문자에 대한 마스크 생성

    // 초기화: 모든 비트를 1로 설정
    for (auto& mask : masks) {
        mask.set(); // 모든 비트를 1로 설정
    }

    // 각 문자 위치에 따라 마스크 업데이트
    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];
        masks[ch].reset(i); // i번째 비트를 0으로 설정
    }

    return masks;
}

// Shift-OR Algorithm for pattern matching
bool shiftOrMatch(const string& text, const string& pattern) {
    int m = pattern.size();  // Pattern length
    vector<bitset<totalBits> > mask = generateMasks(pattern);


    bitset<totalBits> state; // Initial state (all bits set to 1)
    state.set();
    cout << "Initial State: " << bitset<totalBits>(state) << endl;

    for (char ch : text) {
        // 현재 문자의 마스크를 적용한 상태 갱신
        state = (state << 1); // 오른쪽에 0 추가
        state |= mask[ch];       // 현재 문자의 마스크와 AND 연산
        cout << "Character: '" << ch << "' | Mask: " << bitset<totalBits>(mask[ch])
            << " | State: " << bitset<totalBits>(state) << endl;
    }

    // // 매칭 조건: m번째 비트가 0인지 확인
    // unsigned int matchBit = 1 << (m - 1); // m번째 비트 위치 계산
    // if ((state & matchBit) == 0) { // m번째 비트가 0이면 매칭
    //     cout << "Matched State: " << bitset<totalBits>(state) << endl;
    //     return true;
    // }
     // 매칭 조건: 마지막 m번째 비트가 0인지 확인
    bitset<totalBits> matchBit;
    matchBit.set(m - 1); // m번째 비트를 1로 설정
    if ((state & matchBit) == 0) {
        cout << "Matched State: " << state << endl;
        return true;
    }

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
        for (const auto& prefix : prefixes) {
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


int main() {
    string text = "agcgt";
    int flag = 0;
    // Generate masks for the input text
    vector<bitset<totalBits> > masks = generateMasks(text);

    string originalText = "CTGCTTTGCtgctcagcatggctgggaggcacAGTGGAAGATCATGCATCCTTCCCCTGGGACTCCTCTGCCAGAGCCTGAGAGCTTTCTCCTGCACACAGGCTAGGGGTAGGGCAGTTGGAATTGATCCATGCCTTCTAGCTAGACTGTGGGTCCCCTCAGTCTTGGGCATGGTGACAGCCCAGCATCAGACAGAGGTCAGTatcaaactagaaaatttaataaatgctgtcaGATTTGTAGACCC";
    string comparisonText;
    ifstream inputFile("/Users/a1/algorithm/out_put_0.txt");
    
    int prefixSize = 100; // Example prefix size
    
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to open file." << endl;
        return 1;
    }
    // Read content of the file into originalText
    getline(inputFile, comparisonText, '\0'); // Read entire file into originalText
    inputFile.close();

    if (comparisonText.empty()) {
        cerr << "Error: File is empty or not read properly." << endl;
        return 1;
    }

    cout << "Original Text: " << originalText << endl;
    // Generate prefixes from the original text
    vector<string> prefixes = generatePrefixes(originalText, prefixSize);

    // Prefix Comparison (1차 검증)
    vector<int> startPositions = prefixComparison(comparisonText, prefixes);
    if (startPositions.empty()) {
        cout << "No matches found in prefix comparison." << endl;
        return 0;
    }
    else{
        for (int startPos : startPositions)
            cout<< "matchposition: " << startPos << endl;
    }

    // Shift-OR Matching (2차 검증)
    for (int startPos : startPositions) {
        // Compare only up to the length of the original text
        //cout << "matchposition: " << startPos << endl;
        string subText = comparisonText.substr(startPos, originalText.size());
        cout << "\nTesting SubText: " << subText << endl;
        if (shiftOrMatch(subText, originalText)) {
            cout << "Pattern matched at position: " << startPos << endl;
            flag =1;
        }
    }
    if(!flag){
        cout << "No matches found." << endl;}
    return 0;
}
