#include "DnaGenerator.h"

int main() {
    std::string matrixFile = "transition_matrix.txt";
    std::vector<std::vector<double> > transitionProb(4, std::vector<double>(4, 0.0));

    // 기존 저장된 전이 행렬이 있으면 불러오기
    std::ifstream checkFile(matrixFile);
    if (checkFile) {
        std::cout << "Loading existing transition matrix from '" << matrixFile << "'.\n";
        transitionProb = loadTransitionMatrix(matrixFile);
    } else {
        std::cerr << "Error: Transition matrix not found. Please provide or generate it.\n";
        return 1;
    }

    // DNA 서열 길이와 개수 입력
    int n = 100; // 생성할 서열 길이
    int m = 10;  // 생성할 서열 개수

    // 서열 생성
    std::vector<std::string> sequences = generateRandomDNASequences(transitionProb, n, m);

    // 생성된 서열 출력
    for (size_t i = 0; i < sequences.size(); ++i) {
        std::cout << "Sequence " << (i + 1) << ": " << sequences[i] << "\n";
    }

    return 0;
}