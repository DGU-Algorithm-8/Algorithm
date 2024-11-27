#include <iostream>
#include <string>
#include <vector>
#include <bitset> // For binary representation
using namespace std;
const int totalBits = 8;
// Shift-OR Algorithm for pattern matching
bool shiftOrMatch(const string& text, const string& pattern) {
    int m = pattern.size();  // Pattern length
    vector<unsigned int> mask(256, ~0); // Initialize bit masks for each character (default is all 1s)

    // Manually define masks for specific characters
    mask['E'] = 0b11111110;
    mask['a'] = 0b11111101;
    mask['r'] = 0b11111011;
    mask['t'] = 0b11110111;
    mask['h'] = 0b11101111;


    unsigned int state = ~0; // Initial state (all bits set to 1)
    cout << "Initial State: " << bitset<8>(state) << endl;

    for (char ch : text) {
        // Shift the previous state and apply the mask of the current character
        state = (state << 1);
        state = state | mask[ch];

        // Print state and character mask
        cout<< "Character: '" << ch << "' | Mask: " << bitset<8>(mask[ch])
            << " | State: " << bitset<8>(state) << endl;
        
        
    }
    
    // 일치 조건: `n`번째 비트만 0이고 나머지는 모두 1인지 확인
    if ((state & (1 << (8 - m))) == 0 && (state | (1 << (8 - m))) == ~0u) {
        cout << "Matched State: " << bitset<8>(state) << endl;
        return true; // Match found
    }

    return false; // No match
}
// Function to generate masks for each character in the input text
vector<unsigned long long> generateMasks(const string& text) {
    int textLength = text.size();
    vector<unsigned long long> masks(256, ~0ULL); // Initialize masks for all characters with all bits set to 1

    // For each character in the text, update its mask
    for (int i = 0; i < textLength; ++i) {
        char ch = text[i];
        masks[ch] &= ~(1ULL << i); // Set the bit corresponding to the character's index to 0
    }

    return masks;
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
    for (size_t i = 0; i <= text.size(); ++i) {
        for (const auto& prefix : prefixes) {
            if (text.substr(i, prefix.size()) == prefix) {
                matchPositions.push_back(i);
                break; // Avoid duplicate matches
            }
        }
    }
    return matchPositions;
}

int main() {
    string text = "agcgt";

    // Generate masks for the input text
    vector<unsigned long long> masks = generateMasks(text);

    // Print masks for each character in the input text
    cout << "Masks for each character in the text:" << endl;
    for (char ch : text) {
        cout << "Character: '" << ch << "' | Mask: " << bitset<totalBits>(masks[ch]) << endl;
    }
    string originalText = "Earth";
    string comparisonText = "sdaasdaEarthasdad";
    int prefixSize = 3; // Example prefix size
    string pattern = "arth"; // Example pattern

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
        if (shiftOrMatch(subText, pattern)) {
            cout << "Pattern matched at position: " << startPos << endl;
            return 0;
        }
    }
    cout << "No matches found." << endl;
    return 0;
}
