#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <map>
using namespace std;

char getNucleotideFromMatrix(const vector<vector<double> >& matrix, char currentBase) {
    static const map<char, int> baseIndex = {{'A', 0}, {'T', 1}, {'C', 2}, {'G', 3}};
    static const map<int, char> indexBase = {{0, 'A'}, {1, 'T'}, {2, 'C'}, {3, 'G'}};

    auto it = baseIndex.find(currentBase);
    if (it == baseIndex.end()) {
        cerr << "Invalid base encountered: " << currentBase << endl;
        return currentBase; // Return as-is for invalid bases
    }

    int currentIndex = it->second; // Get index for the current base
    vector<double> probs = matrix[currentIndex];
    vector<double> cumulative(4, 0.0);

    // Cumulative probability calculation
    cumulative[0] = probs[0];
    for (int i = 1; i < 4; ++i) {
        cumulative[i] = cumulative[i - 1] + probs[i];
    }

    // Generate a random number to select the next base
    random_device rd;
    mt19937 rng(rd());
    uniform_real_distribution<double> dist(0.0, 1.0);
    double randValue = dist(rng);

    for (int i = 0; i < 4; ++i) {
        if (randValue <= cumulative[i]) {
            return indexBase.at(i); // Use 'at' to access map safely
        }
    }
    return currentBase; // Fallback (should not happen)
}

string introduceDifferencesWithMatrix(const string& genome, double differenceRate, const vector<vector<double> >& matrix) {
    size_t genomeSize = genome.size();
    size_t numDifferences = static_cast<size_t>(genomeSize * differenceRate);
    unordered_set<size_t> modifiedIndices;

    string modifiedGenome = genome;

    while (modifiedIndices.size() < numDifferences) {
        size_t randomIndex = rand() % genomeSize;
        if (modifiedGenome[randomIndex] == 'A' || modifiedGenome[randomIndex] == 'T' ||
            modifiedGenome[randomIndex] == 'C' || modifiedGenome[randomIndex] == 'G') {
            modifiedGenome[randomIndex] = getNucleotideFromMatrix(matrix, modifiedGenome[randomIndex]);
            modifiedIndices.insert(randomIndex);
        }
    }

    return modifiedGenome;
}

string readGenomeFromFile(const string& filename) {
    ifstream input(filename);
    if (!input.is_open()) {
        cerr << "Error: Unable to open file " << filename << endl;
        exit(1);
    }

    string genome;
    string line;

    while (getline(input, line)) {
        if (line[0] == '>') continue; // Skip FASTA headers
        genome += line;
    }

    input.close();
    return genome;
}

vector<vector<double> > loadTransitionMatrix(const string& filename) {
    vector<vector<double> > matrix(4, vector<double>(4, 0.0));
    ifstream input(filename);
    if (!input.is_open()) {
        cerr << "Error: Unable to load transition matrix from file " << filename << endl;
        exit(1);
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            input >> matrix[i][j];
        }
    }

    input.close();
    return matrix;
}

int main() {
    string inputFile = "GCF_009914755.1_T2T-CHM13v2.0_genomic.fna";
    string outputFile = "modified_genome.txt";
    string matrixFile = "transition_matrix.txt";
    double differenceRate = 0.01; // 1% difference

    string genome = readGenomeFromFile(inputFile);
    vector<vector<double> > transitionMatrix = loadTransitionMatrix(matrixFile);

    transform(genome.begin(), genome.end(), genome.begin(), ::toupper);

    string modifiedGenome = introduceDifferencesWithMatrix(genome, differenceRate, transitionMatrix);

    ofstream output(outputFile);
    if (!output.is_open()) {
        cerr << "Error: Unable to write to file " << outputFile << endl;
        return 1;
    }

    output << modifiedGenome;
    output.close();

    cout << "Modified genome saved to " << outputFile << endl;
    return 0;
}
