#include <iostream>
#include <string>
#include <vector>
#include <bitset> // For binary representation
#include <math.h>
#include <fstream>

using namespace std;
const int totalBits = 60;
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

// // Shift-OR Algorithm for pattern matching
// bool shiftOrMatch(const string& text, const string& pattern) {
//     int m = pattern.size();  // Pattern length
//     vector<bitset<totalBits> > mask = generateMasks(pattern);


//     bitset<totalBits> state; // Initial state (all bits set to 1)
//     state.set();
//     cout << "Initial State: " << bitset<totalBits>(state) << endl;

//     for (char ch : text) {
//         // 현재 문자의 마스크를 적용한 상태 갱신
//         state = (state << 1); // 오른쪽에 0 추가
//         state |= mask[ch];       // 현재 문자의 마스크와 OR 연산
//         // cout << "Character: '" << ch << "' | Mask: " << bitset<totalBits>(mask[ch])
//         //     << " | State: " << bitset<totalBits>(state) << endl;
//     }

//     // // 매칭 조건: m번째 비트가 0인지 확인
//     // unsigned int matchBit = 1 << (m - 1); // m번째 비트 위치 계산
//     // if ((state & matchBit) == 0) { // m번째 비트가 0이면 매칭
//     //     cout << "Matched State: " << bitset<totalBits>(state) << endl;
//     //     return true;
//     // }
//      // 매칭 조건: 마지막 m번째 비트가 0인지 확인
//     bitset<totalBits> matchBit;
//     matchBit.set(m - 1); // m번째 비트를 1로 설정
//     if ((state & matchBit) == 0) {
//         cout << "Matched State: " << state << endl;
//         return true;
//     }

//     return false;
// }
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
            cout << mismatches << " mismatches." << endl;
            // Mismatch 위치를 기준으로 SNP 대체
            for (int pos : mismatchPositions) {
                cout << "mismatches at pos, replacedText[], pattern[] " << pos << ", "<<replacedText[pos+startpos]<< ", "<<pattern[pos]<< endl;
                replacedText[pos+startpos] = pattern[pos]; // 텍스트를 패턴의 문자로 대체
            }
            return true;
        }
    }

    cout << "No match found within " << maxMismatches << " mismatches." << endl;
    return false;
}
// bool shiftOrWithSNPReplacement(const string& text, const string& pattern, int maxMismatches, string& replacedText) {
//     int m = pattern.size();  // Pattern 길이
//     vector<bitset<totalBits> > mask = generateMasks(pattern);

//     bitset<totalBits> state; // 상태 (초기: 모든 비트 1)
//     state.set();

//     int mismatches = 0; // Mismatch 카운트
//     vector<int> mismatchPositions; // Mismatch 위치 추적

//     replacedText = text; // 텍스트 복사 (대체를 위해)

//     for (size_t i = 0; i < text.size(); ++i) {
//         char ch = text[i];
//         bitset<totalBits> prevState = state;

//         // 현재 문자에 따라 상태 갱신
//         state = (prevState << 1) | mask[ch];

//         // 매칭 실패 처리: 현재 매칭 중인 글자 위치의 비트를 확인
//         if (!state[i]) { // 현재 매칭 위치의 비트가 0이면 실패
//             mismatches++;
//             mismatchPositions.push_back(i); // Mismatch 위치 저장
//             if (mismatches > maxMismatches) {
//                 cout << "Exceeded maximum mismatches at position " << i << endl;
//                 return false; // 최대 허용 Mismatch 초과
//             }
//             // 매칭 실패 시 상태를 초기화 (글자 수에 맞는 유효 상태로 설정)
//             state.set();
//             state.reset(i);
//         }

//         // 상태 출력
//         cout << "Character: '" << ch << "' | State: " << state
//             << " | Mismatches: " << mismatches << endl;

//         bitset<totalBits> matchBit;
//         matchBit.set(m - 1); // m번째 비트를 1로 설정
//         if ((state & matchBit) == 0) {
//             cout << "Pattern matched with " << mismatches << " mismatches." << endl;

//             // Mismatch 위치를 기준으로 SNP 대체
//             for (int pos : mismatchPositions) {
//                 replacedText[pos+startpos] = pattern[pos]; // 텍스트를 패턴의 문자로 대체
//             }

//             return true; // 매칭 성공
//         }
//     }

//     cout << "No match found within " << maxMismatches << " mismatches." << endl;
//     return false;
// }



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


int main() {
    string text = "agcgt";
    int flag = 0;
    // Generate masks for the input text
    vector<bitset<totalBits> > masks = generateMasks(text);
    int prefixSize = 11;
    string originalText = "AGAGGACTCAGTAAAGGCTGTGCTGGCAATAACATCAAACTACTGAATTCTTTAAGAAC";
    string comparisonText;
    string replacedText;
    ifstream inputFile("/Users/a1/algorithm/out_put_0.txt");

    if (!inputFile.is_open()) {
        cerr << "Error: Unable to open file." << endl;
        return 1;
    }

    
    char ch;
    while (inputFile.get(ch)) {           // 한 글자씩 읽기
        comparisonText += toupper(ch);    // 대문자로 변환하여 저장
    }

    inputFile.close();


    if (comparisonText.empty()) {
        cerr << "Error: File is empty or not read properly." << endl;
        return 1;
    }
    replacedText = comparisonText;

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
        if (shiftOrWithMismatches(subText, originalText,replacedText,30,startPos)){
        //if(shiftOrWithSNPReplacement(subText, originalText, 30, replacedText)){
            cout << "Pattern matched at position: " << startPos << endl;
            // 파일로 저장
            string outputPath = "replaced_text.txt";
            saveToFile(outputPath, replacedText);
            flag =1;
        }
    }
    if(!flag){
        cout << "No matches found." << endl;}
    return 0;
}
