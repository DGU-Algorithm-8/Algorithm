#include <iostream>
#include <string>
#include <vector>
using namespace std;

// Shift-OR Algorithm for pattern matching
bool shiftOrMatch(const string& text, const string& pattern) {
    int m = pattern.size();
    vector<unsigned int> mask(256, ~0); // Initialize bit masks for each character

    for (int i = 0; i < m; ++i) {
        mask[pattern[i]] &= ~(1 << i); // Update the bit mask for the pattern
    }

    unsigned int state = ~0; // Initial state
    for (char ch : text) {
        state = (state << 1) | mask[ch];
        if (!(state & (1 << (m - 1)))) {
            return true; // Match found
        }
    }
    return false; // No match
}

// Parallel Prefix Comparison
vector<int> prefixComparison(const string& text, const vector<string>& prefixes) {
    vector<int> matchPositions;
    size_t textSize = text.size();

    for (const auto& prefix : prefixes) {
        size_t prefixSize = prefix.size();
        for (size_t i = 0; i <= textSize - prefixSize; ++i) {
            if (text.substr(i, prefixSize) == prefix) {
                matchPositions.push_back(i);
            }
        }
    }
    return matchPositions;
}

int main() {
    string originalText = "Earth";
    string comparisonText = "sdaasdaearthasdad";
    int prefixSize = 3; // Example prefix size
    string pattern = "arth"; // Example pattern

    // Generate prefixes from both texts
    vector<string> prefixes;
    for (size_t i = 0; i <= originalText.size() - prefixSize; ++i) {
        prefixes.push_back(originalText.substr(i, prefixSize));
    }
    for (size_t i = 0; i <= comparisonText.size() - prefixSize; ++i) {
        prefixes.push_back(comparisonText.substr(i, prefixSize));
    }

    // Prefix Comparison
    vector<int> startPositions = prefixComparison(originalText, prefixes);
    if (startPositions.empty()) {
        cout << "No matches found in prefix comparison." << endl;
        return 0;
    }

    // Shift-OR Matching
    for (int startPos : startPositions) {
        string subText = originalText.substr(startPos);
        if (shiftOrMatch(subText, pattern)) {
            cout << "Pattern matched at position: " << startPos << endl;
            return 0;
        }
    }

    cout << "No matches found." << endl;
    return 0;
}
