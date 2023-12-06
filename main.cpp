#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <thread>
#include <mutex>

struct result{
    result(size_t line_num, size_t line_index, std::string line)
        : line_num(line_num), line_index(line_index), line(std::move(line)){}
    size_t line_num;
    size_t line_index;
    std::string line;
    void show_result() const
    {
        std::cout<<line_num<<" "<<line_index<<" "<<line<<std::endl;
    }
};

bool compare_results(const result& a, const result& b) {
    return a.line_num < b.line_num;
}

bool match_mask(const std::string &str, int start, const std::string &mask) {
    for (size_t i = 0; i < mask.length(); ++i) {
        if (mask[i] != '?' && mask[i] != str[start + i]) {
            return false;
        }
    }
    return true;
}

int count_lines(const std::string& filename, std::vector<std::string>& fileStr) {
    std::ifstream file(filename);
    int count = 0;
    std::string line;

    if (!file) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        return -1;
    }

    while (std::getline(file, line)) {
        fileStr.push_back(line);
        ++count;
    }

    return count;
}

void find_in_segment(const std::vector<std::string>& fileStr, size_t start_line, size_t end_line,
                     const std::string& mask, std::vector<result>& results,
                     std::mutex& results_mutex) {

    for(size_t line_number = start_line; line_number < end_line; line_number++)  {
        if(line_number >= fileStr.size()){
            break;
        }
        const std::string& line = fileStr[line_number];

        for (size_t i = 0; i <= line.length() - mask.length(); ++i) {
            if (match_mask(line, i, mask)) {
                std::lock_guard<std::mutex> lock(results_mutex);
                result res(line_number, i + 1, line.substr(i, mask.length()));
                results.push_back(res);
                break;
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
    std::vector<result> results;
    std::mutex results_mutex;

    size_t num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    std::vector<std::string> file;
    size_t all_lines = count_lines(filename, std::ref(file));
    if(all_lines < num_threads){
        num_threads = all_lines;
    }
    size_t lines_per_thread = std::ceil(all_lines / double (num_threads));

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(find_in_segment, file,
                             i * lines_per_thread, (i + 1) * lines_per_thread,
                             mask, std::ref(results), std::ref(results_mutex));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::sort(results.begin(), results.end(), compare_results);

    std::cout << results.size() << std::endl;
    for (const auto& res : results) {
        res.show_result();
    }

    return 0;
}
