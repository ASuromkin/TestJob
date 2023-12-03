#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <sstream>

bool compare_results(const std::string& a, const std::string& b) {
    std::istringstream issA(a);
    std::istringstream issB(b);
    int lineA, lineB;
    issA >> lineA;
    issB >> lineB;
    return lineA < lineB;
}

bool match_mask(const std::string &str, int start, const std::string &mask) {
    for (size_t i = 0; i < mask.length(); ++i) {
        if (mask[i] != '?' && mask[i] != str[start + i]) {
            return false;
        }
    }
    return true;
}

int count_lines(const std::string& filename) {
    std::ifstream file(filename);
    int count = 0;
    std::string line;

    while (std::getline(file, line)) {
        ++count;
    }

    return count;
}

void find_in_segment(const std::string& filename, int start_line, int end_line,
                     const std::string& mask, std::vector<std::string>& results,
                     std::mutex& results_mutex) {
    std::ifstream file(filename);
    std::string line;
    int line_number = 0;

    while (getline(file, line)) {
        line_number++;
        if (line_number < start_line || line_number > end_line) continue;

        for (size_t i = 0; i <= line.length() - mask.length(); ++i) {
            if (match_mask(line, i, mask)) {
                std::lock_guard<std::mutex> lock(results_mutex);
                results.push_back(std::to_string(line_number) + " " + std::to_string(i + 1) + " " + line.substr(i, mask.length()));
                break; // Для предотвращения пересечений вхождений
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Использование: " << argv[0] << " <filename> <mask>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::string mask = argv[2];
    std::vector<std::string> results;
    std::mutex results_mutex;

    // Пример: 4 потока
    size_t num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    size_t all_lines = count_lines(filename);
    if(all_lines < num_threads){
        num_threads = all_lines;
    }
    size_t lines_per_thread = std::ceil(all_lines / double (num_threads));

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(find_in_segment, filename,
                             i * lines_per_thread + 1, (i + 1) * lines_per_thread,
                             mask, std::ref(results), std::ref(results_mutex));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::sort(results.begin(), results.end(), compare_results);

    std::cout << results.size() << std::endl;
    for (const auto& match : results) {
        std::cout << match << std::endl;
    }

    return 0;
}
