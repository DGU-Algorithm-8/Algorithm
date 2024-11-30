#ifndef dnagenerator_h
#define dna

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include <ctime>

// 염기를 인덱스로 변환
int baseToIndex(char base) {
    switch (base) {
        case 'A': return 0;
        case 'T': return 1;
        case 'C': return 2;
        case 'G': return 3;
        default: return -1;
    }
}

// 염기를 인덱스에서 변환
char indexToBase(int index) {
    switch (index) {
        case 0: return 'A';
        case 1: return 'T';
        case 2: return 'C';
        case 3: return 'G';
        default: return 'N'; // Unknown
    }
}

// 4x4 전이 행렬을 파일에 저장
void saveTransitionMatrix(const std::vector<std::vector<double>>& matrix, const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Error: Unable to save transition matrix to '" << filename << "'.\n";
        return;
    }
    for (const auto& row : matrix) {
        for (size_t j = 0; j < row.size(); ++j) {
            outFile << row[j];
            if (j < row.size() - 1) {
                outFile << " ";
            }
        }
        outFile << "\n";
    }
    outFile.close();
    std::cout << "Transition matrix saved to '" << filename << "'.\n";
}

// 4x4 전이 행렬을 파일에서 읽기
std::vector<std::vector<double>> loadTransitionMatrix(const std::string& filename) {
    std::ifstream inFile(filename);
    std::vector<std::vector<double>> matrix(4, std::vector<double>(4, 0.0));
    if (!inFile) {
        std::cerr << "Error: Unable to load transition matrix from '" << filename << "'.\n";
        return matrix;
    }

    std::string line;
    size_t row = 0;
    while (std::getline(inFile, line) && row < 4) {
        std::istringstream iss(line);
        for (size_t col = 0; col < 4; ++col) {
            iss >> matrix[row][col];
        }
        ++row;
    }
    inFile.close();
    return matrix;
}

// DNA 서열 생성 함수
std::vector<std::string> generateRandomDNASequences(
    const std::vector<std::vector<double>>& transitionProb, int n, int m) {
    std::vector<std::string> sequences;
    std::mt19937 rng(static_cast<unsigned int>(time(0)));

    for (int seqIndex = 0; seqIndex < m; ++seqIndex) {
        std::string generatedSequence;

        // 초기 염기 선택
        std::uniform_int_distribution<int> initialDist(0, 3);
        int currentBase = initialDist(rng);
        generatedSequence += indexToBase(currentBase);

        // n 길이의 DNA 서열 생성
        for (int i = 1; i < n; ++i) {
            std::vector<double> probs = transitionProb[currentBase];
            std::vector<double> cumulative(4, 0.0);

            cumulative[0] = probs[0];
            for (int j = 1; j < 4; ++j) {
                cumulative[j] = cumulative[j - 1] + probs[j];
            }

            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double randNum = dist(rng);

            int nextBase = 0;
            while (nextBase < 4 && randNum > cumulative[nextBase]) {
                nextBase++;
            }
            if (nextBase == 4) nextBase = 3; // 경계 조건

            generatedSequence += indexToBase(nextBase);
            currentBase = nextBase;
        }

        sequences.push_back(generatedSequence);
    }

    return sequences;
}

#endif // DNAPROCESSOR_H
