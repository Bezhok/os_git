#include <fstream>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <functional>

struct Mat {
	// TODO matrix should contains flat array not vectors!
    std::vector<std::vector<int>> m;
    int rows = 0;
    int columns = 0;
};

class Task {
public:
    std::vector<std::thread> pool_;
    int threads_count_;
    Mat mat1_, mat2_;
    Mat res;
    std::vector<int> rangesi;
    std::vector<int> rangesj;
    std::vector<int> rangesk;
    int last_range_start_k;
    int last_range_start_i;
    int last_range_start_j;
    int shifti = 0;
    int shiftj = 0;
    int shiftk = 0;

    void Reset() {
        res.m.clear();
        res.m.resize(mat1_.rows, std::vector<int>(mat2_.columns, 0));
        res.rows = mat1_.rows;
        res.columns = mat2_.columns;

        shiftk = mat1_.columns / threads_count_;
        shifti = mat1_.rows / threads_count_;
        shiftj = mat2_.columns / threads_count_;

        rangesk.clear();
        for (int i = 0; i < threads_count_; ++i) {
            rangesk.push_back((i) * shiftk);
            rangesi.push_back((i) * shifti);
            rangesj.push_back((i) * shiftj);
        }
        last_range_start_k = shiftk * (threads_count_ - 1);
        last_range_start_i = shifti * (threads_count_ - 1);
        last_range_start_j = shiftj * (threads_count_ - 1);

        pool_.clear();
        pool_.reserve(threads_count_);
    }

    void MultSectorA(int i_s, int i_e, int j_s, int j_e) {
        if (i_s == last_range_start_i) i_e = mat1_.rows;
        if (j_s == last_range_start_j) j_e = mat2_.columns;

        for (int i = i_s; i < i_e; ++i) {
            for (int j = j_s; j < j_e; ++j) {
                for (int k = 0; k < mat1_.columns; ++k) {
                    res.m[i][j] += mat1_.m[i][k] * mat2_.m[k][j];
                }
            }
        }
    }

    void MultMatA() {
        auto callback = [&](int i, int j) {
            MultSectorA(rangesi[i], rangesi[i] + shifti, rangesj[j], rangesj[j] + shiftj);
        };

        for (int j = 0; j < threads_count_; ++j) {
            // TODO should be thread pool not this stupid realisation
            pool_.clear();
            pool_.reserve(threads_count_);
            for (int i = 0; i < threads_count_; ++i) {
                pool_.emplace_back(callback, i, j);
            }

            for (auto &t : pool_) {
                t.join();
            }
        }
    }

    void MultSectorB(int k_s, int k_e) {
        if (k_s == last_range_start_k) k_e = mat1_.columns;

        for (int i = 0; i < mat1_.rows; ++i) {
            for (int j = 0; j < mat2_.columns; ++j) {
                for (int k = k_s; k < k_e; ++k) {
                    res.m[i][j] += mat1_.m[i][k] * mat2_.m[k][j];
                }
            }
        }
    }

    void MultMatB() {
        auto callback = [&](int i) {
            MultSectorB(rangesk[i], rangesk[i] + shiftk);
        };

        for (int k = 0; k < threads_count_; ++k) {
            pool_.emplace_back(callback, k);
        }

        for (auto &t : pool_) {
            t.join();
        }
    }

    void MultSectorC(int i_s, int i_e, int j_s, int j_e, int k_s, int k_e) {
        if (i_s == last_range_start_i) i_e = mat1_.rows;
        if (j_s == last_range_start_j) j_e = mat2_.columns;
        if (k_s == last_range_start_k) k_e = mat1_.columns;

        for (int i = i_s; i < i_e; ++i) {
            for (int j = j_s; j < j_e; ++j) {
                for (int k = k_s; k < k_e; ++k) {
                    res.m[i][j] += mat1_.m[i][k] * mat2_.m[k][j];
                }
            }
        }
    }

    void MultMatC() {
        auto callback = [&](int i, int j, int k) {
            MultSectorC(rangesi[i], rangesi[i] + shifti, rangesj[j], rangesj[j] + shiftj, rangesk[k], rangesk[k] + shiftk);
        };

        for (int j = 0; j < threads_count_; ++j) {
            for (int i = 0; i < threads_count_; ++i) {
                // TODO should be thread pool not this stupid realisation
                pool_.clear();
                pool_.reserve(threads_count_);
                for (int k = 0; k < threads_count_; ++k) {
                    pool_.emplace_back(callback, i, j, k);
                }

                for (auto &t : pool_) {
                    t.join();
                }
            }
        }
    }

    void MultMat() {
        for (int i = 0; i < mat1_.rows; ++i) {
            for (int j = 0; j < mat2_.columns; ++j) {
                for (int k = 0; k < mat1_.columns; ++k) {
                    res.m[i][j] += mat1_.m[i][k] * mat2_.m[k][j];
                }
            }
        }
    }

    void Read() {
        std::ifstream f;
        f.open("input.txt");
        f >> threads_count_;
        pool_.resize(threads_count_);
        for (int i = 0; i < threads_count_; ++i) {

        }

        ReadMat(f, mat1_);
        ReadMat(f, mat2_);

        if (mat1_.columns != mat2_.rows) {
            throw 1;
        };
    }

    void ReadMat(std::ifstream &f, Mat &m) {
        f >> m.rows;
        f >> m.columns;

        m.m.resize(m.rows, std::vector<int>(m.columns, 0));

        for (int i = 0; i < m.rows; ++i) {
            for (int j = 0; j < m.columns; ++j) {
                f >> m.m[i][j];
            }
        }
    }

    void Print() {
        for (int i = 0; i < res.rows; ++i) {
            for (int j = 0; j < res.columns; ++j) {
                std::cout << res.m[i][j] << ' ';
            }
            std::cout << std::endl;
        }
    }
};

void timer(std::function<void()> callBack, Task &t) {
    std::clock_t start;
    double duration;

    t.Reset();
    start = std::clock();

    callBack();

    duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    std::cout << "dur: " << duration << '\n';
    t.Print();
    std::cout << '\n';
}

int main() {
    Task t;
    t.Read();

    timer([&]() {t.MultMat();}, t);
    timer([&]() {t.MultMatA();}, t);
    timer([&]() {t.MultMatB();}, t);
    timer([&]() {t.MultMatC();}, t);

    return 0;
}