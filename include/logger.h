#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <iomanip>
#include <chrono>

namespace Logger {
    // ANSII colors
    const std::string RESET   = "\033[0m";
    const std::string GREEN   = "\033[1;32m";
    const std::string BLUE    = "\033[1;34m";
    const std::string RED     = "\033[1;31m";
    const std::string YELLOW  = "\033[1;33m";
    const std::string CLEAR_LINE = "\033[K";

    inline void task_start(const std::string& msg) {
        std::clog << BLUE << "[INFO] " << RESET << msg << " ... " << std::flush;
    }

    inline void task_end(const std::string& status = "Done.") {
        std::clog << status << std::endl;
    }

    inline void info(const std::string& msg) {
        std::clog << BLUE << "[INFO] " << RESET << msg << std::endl;
    }

    inline void error(const std::string& msg) {
        std::cerr << RED << "[ERROR] " << RESET << msg << std::endl;
    }

    inline void update_progress(int done, int total, std::chrono::steady_clock::time_point start) {
        int bar_width = 40;
        float progress = static_cast<float>(done) / total;
        int pos = static_cast<int>(bar_width * progress);
        int percent = static_cast<int>(progress * 100);

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        double remaining = (progress > 0) ? (elapsed.count() / progress) - elapsed.count() : 0;

        std::clog << "\r" << CLEAR_LINE << GREEN << "Rendering " << RESET << "[";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::clog << "#";
            else if (i == pos) std::clog << ">";
            else std::clog << " ";
        }
        std::clog << "] " << std::setw(3) << percent << "% | "
                  << "ET: " << std::fixed << std::setprecision(1) << elapsed.count() << "s | "
                  << "ETA: " << std::fixed << std::setprecision(1) << remaining << "s" << std::flush;
    }

    inline void finish_phase(const std::string& msg = "") {
        std::clog << "\n"; 
        if(!msg.empty()) std::clog << BLUE << "[INFO] " << RESET << msg << std::endl;
    }

    inline void finish_render(std::chrono::steady_clock::time_point start) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        
        std::clog << "\n" << BLUE << "[INFO] " << RESET 
                << "Rendering complete in " << std::fixed << std::setprecision(1) 
                << elapsed.count() << "s." << std::endl;
    }

    inline void success(const std::string& msg) {
        std::clog << GREEN << "[SUCCESS] " << RESET << msg << std::endl;
    }

    inline void stage(const std::string& name) {
        std::clog << "\n" << BLUE << "== " << RESET << YELLOW << name << RESET << BLUE << " ==" << RESET << std::endl;
    }
}
#endif