#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <iomanip>
#include <chrono>
#include <sstream>

namespace Logger {
    // ANSI colors
    const std::string RESET      = "\033[0m";
    const std::string GREEN      = "\033[1;32m";
    const std::string BLUE       = "\033[1;34m";
    const std::string RED        = "\033[1;31m";
    const std::string YELLOW     = "\033[1;33m";
    const std::string PURPLE     = "\033[1;35m";
    const std::string DIM        = "\033[2m";
    const std::string CLEAR_LINE = "\033[2K";
    const std::string CURSOR_UP  = "\033[1A";
    const std::string CURSOR_COL = "\033[1G";

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

    inline void success(const std::string& msg) {
        std::clog << GREEN << "[SUCCESS] " << RESET << msg << std::endl;
    }

    inline void stage(const std::string& name) {
        std::clog << "\n" << BLUE << "== " << RESET << YELLOW << name << RESET << BLUE << " ==" << RESET << "\n";
    }

    inline void finish_phase(const std::string& msg = "") {
        std::clog << "\n";
        if (!msg.empty())
            std::clog << BLUE << "[INFO] " << RESET << msg << "\n";
    }

    inline void finish_render(std::chrono::steady_clock::time_point start) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        std::clog << "\n" << BLUE << "[INFO] " << RESET
                  << "Rendering complete in "
                  << std::fixed << std::setprecision(1)
                  << elapsed.count() << "s.\n";
    }

    inline void update_progress(int done, int total,
                                std::chrono::steady_clock::time_point start)
    {
        const int bar_width = 40;
        float progress = static_cast<float>(done) / total;
        int   pos      = static_cast<int>(bar_width * progress);
        int   percent  = static_cast<int>(progress * 100);

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed   = now - start;
        double remaining = (progress > 0)
            ? (elapsed.count() / progress) - elapsed.count() : 0;

        std::clog << "\r" << CLEAR_LINE
                  << GREEN << "Rendering " << RESET << "[";
        for (int i = 0; i < bar_width; ++i) {
            if      (i < pos)  std::clog << "#";
            else if (i == pos) std::clog << ">";
            else               std::clog << " ";
        }
        std::clog << "] " << std::setw(3) << percent << "% | "
                  << "ET: "  << std::fixed << std::setprecision(1) << elapsed.count()  << "s | "
                  << "ETA: " << std::fixed << std::setprecision(1) << remaining        << "s"
                  << std::flush;
    }

    inline void anim_progress_init(int total_frames, int fps,
                                   double duration_s, int width, int spp)
    {
        std::clog << DIM
                  << total_frames << " frames  |  "
                  << fps          << " fps  |  "
                  << std::fixed << std::setprecision(1) << duration_s << "s  |  "
                  << width << "x" << width << "  |  "
                  << spp << " SPP"
                  << RESET << "\n"
                  << "\n"
                  << "\n"
                  << "\n";
        std::clog << std::flush;
    }

    inline std::string make_bar(const std::string& label,
                                const std::string& color,
                                int done, int total,
                                const std::string& annotation,
                                int bar_width = 36)
    {
        float pct = (total > 0) ? static_cast<float>(done) / total : 0.f;
        int   pos = static_cast<int>(bar_width * pct);

        std::ostringstream ss;
        ss << color << label << RESET << " [";
        for (int i = 0; i < bar_width; ++i) {
            if      (i < pos)  ss << "#";
            else if (i == pos) ss << ">";
            else               ss << " ";
        }
        ss << "] " << std::setw(3) << static_cast<int>(pct * 100) << "%  "
           << DIM << annotation << RESET;
        return ss.str();
    }

    inline void update_anim_progress(
        int    frames_done,
        int    frame_total,
        const std::string& current_name,
        std::chrono::steady_clock::time_point anim_start,
        std::chrono::steady_clock::time_point frame_start,
        int frame_rows_done  = 0, int frame_rows_total = 1)
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> anim_elapsed  = now - anim_start;
        std::chrono::duration<double> frame_elapsed = now - frame_start;

        double anim_pct   = (frame_total > 0)
                            ? static_cast<double>(frames_done) / frame_total : 0.0;
        double anim_eta   = (anim_pct > 0)
                            ? (anim_elapsed.count() / anim_pct) - anim_elapsed.count() : 0.0;
        double avg        = (frames_done > 0)
                            ? anim_elapsed.count() / frames_done : 0.0;

        std::ostringstream oa, fa, stats;
        oa << "frame " << frames_done << " / " << frame_total
           << "  ETA " << std::fixed << std::setprecision(0) << anim_eta << "s";

        fa << current_name << "  "
           << std::fixed << std::setprecision(1) << frame_elapsed.count() << "s";

        if (avg > 0)
            stats << DIM << "avg/frame " << std::fixed << std::setprecision(1) << avg
                  << "s  |  elapsed " << std::fixed << std::setprecision(0)
                  << anim_elapsed.count() << "s" << RESET;

        std::clog
            << "\033[3A"
            << CURSOR_COL << CLEAR_LINE
            << make_bar(PURPLE + "Overall" + RESET, PURPLE,
                        frames_done, frame_total, oa.str())
            << "\n" << CURSOR_COL << CLEAR_LINE
            << make_bar(GREEN + "Frame  " + RESET, GREEN,
                        frame_rows_done, frame_rows_total, fa.str())
            << "\n" << CURSOR_COL << CLEAR_LINE
            << stats.str()
            << "\n"
            << std::flush;
    }

    inline void anim_progress_finish(int total_frames,
                                     std::chrono::steady_clock::time_point anim_start)
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - anim_start;
        double avg = elapsed.count() / total_frames;

        std::clog << "\n"
                  << BLUE << "[INFO] " << RESET
                  << "Animation complete | "
                  << total_frames << " frames in "
                  << std::fixed << std::setprecision(1) << elapsed.count() << "s  "
                  << DIM << "(avg " << std::setprecision(2) << avg << "s/frame)" << RESET
                  << "\n";
    }
}

#endif