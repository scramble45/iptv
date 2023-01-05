#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <experimental/filesystem>
#include <ncurses.h>

namespace fs = std::experimental::filesystem;

const int ITEMS_PER_PAGE = 25;

int main(int argc, char * argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " directory" << std::endl;
    return 1;
  }

  std::string directory = argv[1];

  std::vector < std::string > channels;
  std::vector < std::string > urls;

  std::map < std::string, std::string > channel_map;

  // Iterate through all .m3u files
  for (const auto & file: fs::directory_iterator(directory)) {
    if (file.path().extension() == ".m3u") {
      // Open a file
      std::ifstream stream(file.path().string());
      std::string line;
      while (std::getline(stream, line)) {
        if (line.find("#EXTINF") == 0) {
          std::string title = line.substr(line.find(',') + 1);
          std::getline(stream, line);
          channels.emplace_back(title);
          urls.emplace_back(line);
          channel_map[title] = line;
        }
      }
    }
  }

  if (system("command -v mpv >/dev/null 2>&1") != 0) {
    std::cerr << "Error: mpv is not installed or not in the PATH" << std::endl;
    return 1;
  }

  // ncurses
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  int rows, cols;

  getmaxyx(stdscr, rows, cols);
  WINDOW * menu_win = newwin(rows, cols, 0, 0);

  int current_page = 1;
  int current_item = 0;
  int num_pages = (channels.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;

  nodelay(stdscr, TRUE);
  
  std::string channel_number_str;

  while (true) {
    werase(menu_win);
    mvwprintw(menu_win, 0, 0, "Channels: ");
    for (int i = (current_page - 1) * ITEMS_PER_PAGE; i < current_page * ITEMS_PER_PAGE && i < channels.size(); ++i) {
      if (i == current_item) {
        wattron(menu_win, A_REVERSE);
      }
      mvwprintw(menu_win, i - (current_page - 1) * ITEMS_PER_PAGE + 1, 0, "%d: %s", i + 1, channels[i].c_str());
      wattroff(menu_win, A_REVERSE);
    }
    mvwprintw(menu_win, rows - 1, 0, "Page %d/%d (Press Q to quit, N for next page, P for previous page, Up/Down to scroll, Enter to select)", current_page, num_pages);
    if (!channel_number_str.empty()) {
      mvwprintw(menu_win, rows - 1, cols - channel_number_str.size() - 1, channel_number_str.c_str());
    }
    wrefresh(menu_win);

    // Get user input
    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
      break;
    } else if (ch == 'n' || ch == 'N' || ch == KEY_RIGHT) {
      if (current_page < num_pages) {
        ++current_page;
        current_item = (current_page - 1) * ITEMS_PER_PAGE;
      }
    } else if (ch == 'p' || ch == 'P' || ch == KEY_LEFT) {
      if (current_page > 1) {
        --current_page;
        current_item = std::min(current_page * ITEMS_PER_PAGE - 1, (int) channels.size() - 1);
      }
    } else if (ch == KEY_UP) {
      if (current_item > 0) {
        --current_item;
        if (current_item < (current_page - 1) * ITEMS_PER_PAGE) {
          --current_page;
          current_item = std::min(current_page * ITEMS_PER_PAGE - 1, (int) channels.size() - 1);
        }
      }
    } else if (ch == KEY_DOWN) {
      if (current_item < channels.size() - 1) {
        ++current_item;
        if (current_item >= current_page * ITEMS_PER_PAGE) {
          ++current_page;
          current_item = (current_page - 1) * ITEMS_PER_PAGE;
        }
      }
    } else if (isdigit(ch)) {
      channel_number_str += ch;
      int channel_number = atoi(channel_number_str.c_str());
      if (channel_number > 0 && channel_number <= channels.size()) {
        current_page = (channel_number - 1) / ITEMS_PER_PAGE + 1;
        current_item = channel_number - 1;
      }
    } else if (ch == KEY_BACKSPACE || ch == 127) {
      if (!channel_number_str.empty()) {
        channel_number_str.pop_back();
      }
    } else if (ch == '\n') {
      std::string url = channel_map[channels[current_item]];
      std::string command = "mpv \"" + url + "\" >/dev/null 2>&1 &";
      int status = system(command.c_str());

      if (status != 0) {
        std::cerr << "Error: Failed to play URL" << std::endl;
        return 1;
      }
    }
  }

  endwin();
  return 0;
}