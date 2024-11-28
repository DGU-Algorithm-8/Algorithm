#include <iostream>
#include <string>
#include <vector>
#include <bitset> // For binary representation
#include <math.h>
using namespace std;
const int totalBits = 8;
const double initialmask = pow(2, totalBits);
// Function to generate masks for each character in the input text
vector<unsigned int> generateMasks(const string& text) {
    int textLength = text.size();
    vector<unsigned int> masks(initialmask, ~0ULL); // Initialize masks for all characters with all bits set to 1

    // For each character in the text, update its mask
    for (int i = 0; i < textLength; ++i) {
        char ch = text[i];
        masks[ch] &= ~(1ULL << i); // Set the bit corresponding to the character's index to 0
    }
    cout << "Masks for each character in the text:" << endl;
    for (char ch : text) {
        cout << "Character: '" << ch << "' | Mask: " << bitset<totalBits>(masks[ch]) << endl;
    }

    return masks;
}
// Shift-OR Algorithm for pattern matching
bool shiftOrMatch(const string& text, const string& pattern) {
    int m = pattern.size();  // Pattern length
    vector<unsigned int> mask = generateMasks(pattern);


    unsigned int state = ~0; // Initial state (all bits set to 1)
    cout << "Initial State: " << bitset<totalBits>(state) << endl;

    for (char ch : text) {
        // 현재 문자의 마스크를 적용한 상태 갱신
        state = (state << 1) | 0; // 오른쪽에 1 추가
        state |= mask[ch];       // 현재 문자의 마스크와 AND 연산
        cout << "Character: '" << ch << "' | Mask: " << bitset<totalBits>(mask[ch])
            << " | State: " << bitset<totalBits>(state) << endl;
    }

    // 매칭 조건: m번째 비트가 0인지 확인
    unsigned int matchBit = 1 << (m - 1); // m번째 비트 위치 계산
    if ((state & matchBit) == 0) { // m번째 비트가 0이면 매칭
        cout << "Matched State: " << bitset<totalBits>(state) << endl;
        return true;
    }

    return false;
}


// Generate prefixes of a given size from the text
vector<string> generatePrefixes(const string& text, int prefixSize) {
    vector<string> prefixes;
    for (size_t i = 0; i <= text.size() - prefixSize; ++i) {
        prefixes.push_back(text.substr(i, prefixSize));
    }
    return prefixes;
}

// Prefix Comparison (1차 검증)
vector<int> prefixComparison(const string& text, const vector<string>& prefixes) {
    vector<int> matchPositions;
    size_t prefixSize = prefixes[0].size(); // prefix 크기 고정 (모든 prefix가 동일 길이라고 가정)
    
    for (size_t i = 0; i + prefixSize <= text.size(); ++i) { // 범위 체크
        bool matched = false;
        for (const auto& prefix : prefixes) {
            if (text.substr(i, prefixSize) == prefix) {
                matchPositions.push_back(i); // 시작 지점만 저장
                matched = true; // 매칭된 상태 저장
                break; // 중복 비교 방지
            }
        }
        if (matched) {
            i += prefixSize - 1; // 매칭된 위치 이후로 건너뜀
        }
    }
    return matchPositions;
}


int main() {
    string text = "agcgt";
    int flag = 0;
    // Generate masks for the input text
    vector<unsigned int> masks = generateMasks(text);

    
    string originalText = "Earth";
    string comparisonText = "sdaasdaEarthasdaEarthd";
    int prefixSize = 3; // Example prefix size
    

    // Generate prefixes from the original text
    vector<string> prefixes = generatePrefixes(originalText, prefixSize);

    // Prefix Comparison (1차 검증)
    vector<int> startPositions = prefixComparison(comparisonText, prefixes);
    if (startPositions.empty()) {
        cout << "No matches found in prefix comparison." << endl;
        return 0;
    }
    
    // Shift-OR Matching (2차 검증)
    for (int startPos : startPositions) {
        // Compare only up to the length of the original text
        
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
