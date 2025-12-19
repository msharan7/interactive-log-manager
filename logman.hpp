// Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3251350
#include <string>
#include <vector>
#include <unordered_map>
#ifndef LOGMAN_HPP
#define LOGMAN_HPP
using namespace std;


struct Entry {
    string ts; // keeping this in struct for purposes of printing
    uint64_t ts_num; // keeping this for efficient comparison
    string cat; // keep for printing
    string msg; // keep for printing
    string lower_cat; // keeping this for efficient comparison
    int id; // id has an important purpose for various reasons
};

class Logman {
public:
    void getOptions(int argc, char **argv);
    void readFile();
    void readUserInput();
private:
    void printHelp(char *command);
    uint64_t convert_to_num(string & ts);
    void a_command(int x);
    void p_command();
    void t_command(string & l, string & u);
    void m_command(string & ts);
    void c_command(string & category);
    void d_command(int pos);
    void b_command(int pos);
    void e_command(int pos);
    void l_command();
    void s_command();
    void g_command();
    void r_command();
    void k_command(string & key);
    void add_to_map(string & s, int id);
    string master_log_file; 
    int id_tracker = 0;
    vector<Entry> entries; // master log. The index and entry IDs are always the same in this vector
    vector<int> excerpt_list; // only contains entry IDs. 
    vector<int> sorted; // only IDs. Uses ids to access entries and sorts based on entry info. 
    vector<int> final; // for keyword printing, just IDs
    unordered_map<string, vector<int>> category_results; // key: category, value: vector of IDs for entries that have that category
    unordered_map<string, vector<int>> keyword_results; // key: word from category or message, value: vector of IDs for entries that have that word
    // this was needed for time optimization in s and k. Allows me to access sorted positions without having to compare strings again
    vector<int> curr_id_pos;
    bool prev_search_exists = false; // for functions like g and r
    char last_search; // for g and r
    // tracks last high and low bound for previous t and m commands
    vector<int>::iterator last_lo;
    vector<int>::iterator last_hi;
    const vector<int> * last_cat_vec = nullptr; // pointer to the value (vector) that correponds to the category that was previously searched for
};

#endif