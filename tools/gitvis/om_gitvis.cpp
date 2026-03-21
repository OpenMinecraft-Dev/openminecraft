#include <array>
#include <cstdlib>
#include <fstream>
#define TB_IMPL
#include "fmt/format.h"
#include "termbox2/termbox2.h"
#include <chrono>
#include <cstdlib> // for std::abs
#include <cstring>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

// Helper: check if a year is a leap year (Gregorian calendar)
bool isLeap(int year)
{
    return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

// Helper: total days in a given year (365 or 366)
int daysInYear(int year)
{
    return isLeap(year) ? 366 : 365;
}

// Compute total days from January 1, year 1 to the start of the given year.
// Equivalent to the sum of days in all years from 1 to year-1.
long long daysBeforeYear(int year)
{
    long long total = 0;
    for (int y = 1; y < year; ++y)
    {
        total += daysInYear(y);
    }
    return total;
}

// Compute total days from epoch (Jan 1, year 1) for a date encoded as year*1000 + yday
long long daysFromEpoch(int encodedDate)
{
    int year = encodedDate / 1000;
    int yday = encodedDate % 1000; // 0‑based day of year
    return daysBeforeYear(year) + yday;
}

// Number of days between two dates (absolute difference)
long long daysBetween(int date1, int date2)
{
    long long days1 = daysFromEpoch(date1);
    long long days2 = daysFromEpoch(date2);
    return std::abs(days1 - days2);
}

int printbar(int x, int y, double prog, int showper)
{
    int wid = tb_width();

    tb_printf(x, y, TB_WHITE, TB_DEFAULT, " ");
    x++;

    auto bg = TB_BLACK;
    int ll = static_cast<int>((wid - x - 8) * 8 * prog * (showper == -1 ? 1.0 : 0.75));
    for (int i = 0; i < ll / 8; i++)
    {
        tb_printf(x, y, TB_GREEN, bg, "█");
        x++;
    }

    tb_set_cell(x, y, 0x2588 + 8 - std::max(1, ll % 8), TB_GREEN, bg);
    x++;
    if (showper == -2)
    {
        return x;
    }

    int currx = x;
    while (wid - x > 8)
    {
        tb_set_cell(x, y, ' ', TB_WHITE, showper == -1 ? bg : TB_DEFAULT);
        x++;
    }

    if (showper == -1)
    {
        tb_printf(wid - 5, y, TB_DIM, TB_DEFAULT, "%d%%", static_cast<int>(prog * 100));
    }
    else
    {
        tb_printf(currx + 2, y, TB_WHITE, TB_DEFAULT, "%d", showper);
        return currx + 8;
    }

    return x;
}

int main(int argc, char *argv[])
{
    auto exitcode = std::system("git log --reverse --pretty=format:'%H%x1e%an%x1e%ae%x1e%at%x1e%s%x1f' > temp.bin");

    std::ifstream result("temp.bin", std::ios::binary);

    std::vector<std::array<std::string, 5>> commits = {{}};

    std::string s = "";
    int index = 0;
    while (result.good())
    {
        char c;
        result.read(&c, 1);

        if (c != 0x1e && c != 0x1f)
        {
            if (c != '\n')
            {
                s += c;
            }
        }
        else
        {
            commits.rbegin()->at(index) = s;
            s = "";
            index++;
        }

        if (c == 0x1f && result.peek() != 0x1f && result.good())
        {
            commits.push_back({});
            index = 0;
        }
    }
    result.close();

    std::remove("temp.bin");
    std::sort(commits.begin(), commits.end(), [](std::array<std::string, 5> a, std::array<std::string, 5> b) {
        return std::stoll(a[3]) < std::stoll(b[3]);
    });

    int status = tb_init();
    if (status)
    {
        std::cerr << "tb_init() failed " << status << std::endl;
        return 1;
    }

    tb_clear();

    std::unordered_map<std::string, int> aut;
    std::unordered_map<std::string, std::pair<int, int>> autex;
    int day = -1;

    for (int i = 0; i < commits.size(); i++)
    {
        tb_clear();

        auto timestamp = static_cast<time_t>(std::stoll(commits[i][3].c_str()));
        auto tms = std::localtime(&timestamp);
        int dayc = tms->tm_yday + (tms->tm_year + 1900) * 1000;
        if (dayc != day)
        {
            for (auto &au : autex)
            {
                if (au.second.second == 0)
                {
                    au.second.first += daysBetween(dayc, day) - 1;
                }
                else
                {
                    au.second.first = 0;
                    au.second.second = 0;
                }
            }

            day = dayc;
        }

        tb_print(0, 0, TB_CYAN, TB_DEFAULT, commits[i][4].c_str());
        tb_print(0, 1, TB_DIM, TB_DEFAULT, commits[i][0].c_str());

        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&timestamp));
        tb_printf(0, 2, TB_YELLOW, TB_DEFAULT, buffer);
        tb_printf(std::strlen(buffer) + 3, 2, TB_RED, TB_DEFAULT, "%d Commits", i + 1);

        if (!aut.count(commits[i][1]))
        {
            aut[commits[i][1]] = 0;
            autex[commits[i][1]] = std::make_pair(0, 0);
        }
        aut[commits[i][1]]++;
        autex[commits[i][1]].second++;

        std::vector<std::pair<std::string, int>> tem;
        for (auto &o : aut)
        {
            tem.push_back(o);
        }
        std::sort(tem.begin(), tem.end(),
                  [](std::pair<std::string, int> a, std::pair<std::string, int> b) { return a.second > b.second; });

        int j = 3;
        int cm = -1;
        for (auto &l : tem)
        {
            tb_printf(0, j, TB_WHITE, TB_DEFAULT, "%s ", l.first.c_str());
            int pp;
            if (cm == -1)
            {
                pp = printbar(16, j, 1.0, l.second);
                cm = l.second;
            }
            else
            {
                pp = printbar(16, j, l.second / static_cast<double>(cm), l.second);
            }

            if (autex[l.first].second > 0)
            {
                tb_printf(pp, j, TB_DIM, TB_DEFAULT, "(+%d today)", autex[l.first].second);
            }
            else if (autex[l.first].first > 7)
            {
                tb_printf(pp, j, TB_DIM, TB_DEFAULT, "(relaxed for %d days)", autex[l.first].first);
            }

            j++;

            if (j == tb_height() - 1)
            {
                break;
            }
        }
        printbar(0, tb_height() - 1, (i + 1) / static_cast<double>(commits.size()), -1);
        tb_present();

        std::this_thread::sleep_for(std::chrono::milliseconds(18));
    }

    tb_event e;
    tb_poll_event(&e);

    tb_shutdown();

    return 0;
}
