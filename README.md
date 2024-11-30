# Algorithm
C++ 11 기반 명령어 포함   
g++ -std=c++11 random_generator.cpp -o main   
./main     
다음과 같은 형태로 실행    

# Aho-Corasick 알고리즘
## 소개
DNA 서열 데이에서 여러 패턴을 동시에 검색하기 위해 Aho-Corasick 알고리즘을 사용.
근사매칭을 구현하여 최대 d개의 오차를 허용하여 패턴을 검색.   
또한 멀티스레딩을 활용하여 여러 패턴을 병렬로 처리함으로써 성능을 항상시켰으며, 메모리 최적화를 통해 효율적인 자원 사용을 구현.   
#### 필수 조건
- C++17 이상   
- C++ 컴파일러: C++ 17 표준을 지원하는 컴파일러(g++, clang++)
- POSIX Threads : 멀티스레딩 지원
- DnaGenerator.h : DNA 서열 생성 및 전이 행렬 로딩을 위한 사용자 정의 헤더파일
> 참고: DnaGenerator.h가 프로젝트 디렉토리에 존재하는지 확인할 것   
#### 주요 구성 요소
1. TrieNode 구조체
```
struct TrieNode {
    std::array<TrieNode*, 4> children; // A, T, C, G 자식 노드
    TrieNode* failure; // 실패 함수 포인터
    std::vector<int> output; // 매칭된 패턴의 인덱스 리스트

    TrieNode() : failure(nullptr) {
        children.fill(nullptr); // 모든 자식 노드를 nullptr로 초기화
    }
};
```
2. DnaGenerator
```
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

```
- Markov Chain 개념을 적용하여 DNA 서열을 생성, 각 염기는 이전 염기에 따라 다음 연기로 전이할 확률을 가진다.
