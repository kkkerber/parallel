#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;

// Функція для генерації тестового рядка заданої довжини
string generateString(size_t length) {
    const string pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t patternSize = pattern.size();
    string result(length, ' ');

    for (size_t i = 0; i < length; ++i) {
        result[i] = pattern[i % patternSize];
    }
    return result;
}

// Функція для запису рядка у файл
void writeFile(const string &filename, const string &data) {
    ofstream out(filename, ios::binary);
    if (!out) {
        cerr << "Помилка при відкритті файлу " << filename << " для запису." << endl;
        return;
    }
    out.write(data.data(), data.size());
}

// Функція для зчитування файлу у рядок
string readFile(const string &filename) {
    ifstream in(filename, ios::binary);
    if (!in) {
        cerr << "Помилка при відкритті файлу " << filename << " для зчитування." << endl;
        return "";
    }
    stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Послідовне реверсування рядка
string sequentialReverse(const string &input) {
    string result = input;
    reverse(result.begin(), result.end());
    return result;
}

// Функція для реверсування сегмента рядка (у потоці)
void reverseSegment(const string &input, string &result, size_t start, size_t end) {
    size_t length = input.size();
    for (size_t i = start; i < end; ++i) {
        result[length - i - 1] = input[i];
    }
}

// Паралельне реверсування рядка
string parallelReverse(const string &input, int numThreads) {
    size_t length = input.size();
    string result(length, ' ');

    // Обмежуємо кількість потоків числом логічних ядер
    int maxThreads = thread::hardware_concurrency();
    if (numThreads > maxThreads) numThreads = maxThreads;

    vector<thread> threads;
    size_t lengthPerThread = length / numThreads;
    size_t remainder = length % numThreads;

    size_t start = 0;
    for (int i = 0; i < numThreads; ++i) {
        size_t end = start + lengthPerThread + (i < remainder ? 1 : 0); // Рівномірний розподіл залишку
        threads.emplace_back(reverseSegment, cref(input), ref(result), start, end);
        start = end;
    }

    for (auto &t : threads) {
        t.join();
    }
    return result;
}

int main() {
    vector<int> fileSizesMB = {5, 13, 20, 41, 74, 141, 197, 265};
    int maxThreads = thread::hardware_concurrency();

    cout << "Доступні логічні ядра: " << maxThreads << endl;
    vector<int> threadCounts = {2, 4, 8, 16};

    for (int sizeMB : fileSizesMB) {
        size_t numChars = static_cast<size_t>(sizeMB) * 1000000;
        string filename = "test_" + to_string(sizeMB) + "MB.txt";

        cout << "--------------------------------------------------" << endl;
        cout << "Генерується файл " << filename << " розміром " << sizeMB << " МБ..." << endl;
        string testString = generateString(numChars);
        writeFile(filename, testString);

        string input = readFile(filename);
        if (input.empty()) {
            cerr << "Не вдалося зчитати файл " << filename << endl;
            continue;
        }
        cout << "Файл " << filename << " зчитано успішно, розмір: " << input.size() << " байт" << endl;

        // Послідовний алгоритм
        auto startTimeSeq = chrono::high_resolution_clock::now();
        string seqResult = sequentialReverse(input);
        auto endTimeSeq = chrono::high_resolution_clock::now();
        auto seqDuration = chrono::duration_cast<chrono::milliseconds>(endTimeSeq - startTimeSeq).count();
        cout << "Послідовний алгоритм: " << seqDuration << " ms" << endl;

        // Паралельний алгоритм
        for (int threadsCount : threadCounts) {
            auto startTimePar = chrono::high_resolution_clock::now();
            string parResult = parallelReverse(input, threadsCount);
            auto endTimePar = chrono::high_resolution_clock::now();
            auto parDuration = chrono::duration_cast<chrono::milliseconds>(endTimePar - startTimePar).count();

            if (parResult != seqResult) {
                cerr << "Помилка: результат паралельного реверсування не співпадає з послідовним." << endl;
            }
            cout << "Паралельний алгоритм з " << threadsCount << " потоками: " << parDuration << " ms" << endl;
        }
    }
    cout << "--------------------------------------------------" << endl;
    return 0;
}
