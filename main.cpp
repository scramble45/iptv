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

// Constants for pagination menu
const int ITEMS_PER_PAGE = 25;

int main(int argc, char * argv[]) {
  // Check if a directory was specified as a command line argument
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " directory" << std::endl;
    return 1;
  }

  // Set the directory to the argument passed on the command line
  std::string directory = argv[1];

  // Create vectors to hold the list of channels and URLs
  std::vector < std::string > channels;
  std::vector < std::string > urls;

  // Map to store the relation between channel name and URL
  std::map < std::string, std::string > channel_map;

  // Iterate through all .m3u files in the specified directory
  for (const auto & file: fs::directory_iterator(directory)) {
    if (file.path().extension() == ".m3u") {
      // Open the file
      std::ifstream stream(file.path().string());
      std::string line;
      // Read the file line by line and extract the channel titles and URLs
      while (std::getline(stream, line)) {
        // If the line starts with "#EXTINF", then it contains the title of a channel
        if (line.find("#EXTINF") == 0) {
          // Extract the title by removing everything before the first comma
          std::string title = line.substr(line.find(',') + 1);
          // Read the next line, which should contain the URL
          std::getline(stream, line);
          // Add the title and URL to the vectors
          channels.emplace_back(title);
          urls.emplace_back(line);
          // Add the title and URL to the map
          channel_map[title] = line;
        }
      }
    }
  }

  // Check if the mpv command is available
  if (system("command -v mpv >/dev/null 2>&1") != 0) {
    std::cerr << "Error: mpv is not installed or not in the PATH" << std::endl;
    return 1;
  }

  // Initialize ncursed library
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  // Declare variables "rows" and "cols"
  int rows, cols;

  getmaxyx(stdscr, rows, cols);
  WINDOW * menu_win = newwin(rows, cols, 0, 0);

  int current_page = 1;
  int current_item = 0;
  int num_pages = (channels.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;

  // Use nodelay to make getch non-blocking
  nodelay(stdscr, TRUE);

  std::string channel_number_str;
  while (true) {
    // Display the list of channels in a pagination menu
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

    // Get the user's input
    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
      break;
    } else if (ch == 'n' || ch == 'N' || ch == KEY_RIGHT) {
      if (current_page < num_pages) {
        ++current_page;
        // When moving to the next page, jump to the top of the page
        current_item = (current_page - 1) * ITEMS_PER_PAGE;
      }
    } else if (ch == 'p' || ch == 'P' || ch == KEY_LEFT) {
      if (current_page > 1) {
        --current_page;
        // When moving to the previous page, jump to the bottom of the page
        current_item = std::min(current_page * ITEMS_PER_PAGE - 1, (int) channels.size() - 1);
      }
    } else if (ch == KEY_UP) {
      if (current_item > 0) {
        --current_item;
        // If moving up to the top of the current page, jump to the bottom of the previous page
        if (current_item < (current_page - 1) * ITEMS_PER_PAGE) {
          --current_page;
          current_item = std::min(current_page * ITEMS_PER_PAGE - 1, (int) channels.size() - 1);
        }
      }
    } else if (ch == KEY_DOWN) {
      if (current_item < channels.size() - 1) {
        ++current_item;
        // If moving down to the bottom of the current page, jump to the top of the next page
        if (current_item >= current_page * ITEMS_PER_PAGE) {
          ++current_page;
          current_item = (current_page - 1) * ITEMS_PER_PAGE;
        }
      }
    } else if (isdigit(ch)) {
      channel_number_str += ch;
      int channel_number = atoi(channel_number_str.c_str());
      if (channel_number > 0 && channel_number <= channels.size()) {
        // Jump to the page containing the selected channel
        current_page = (channel_number - 1) / ITEMS_PER_PAGE + 1;
        current_item = channel_number - 1;
      }
    } else if (ch == KEY_BACKSPACE || ch == 127) {
      if (!channel_number_str.empty()) {
        channel_number_str.pop_back();
      }
    } else if (ch == '\n') {
      std::string url = channel_map[channels[current_item]];
      // Launch mpv to play the selected channel
      std::string command = "mpv \"" + url + "\" >/dev/null 2>&1 &";
      int status = system(command.c_str());

      // Check if mpv returned an error
      if (status != 0) {
        std::cerr << "Error: Failed to play URL" << std::endl;
        return 1;
      }
    }
  }

  // Deinitialize ncursed library
  endwin();

  return 0;
}