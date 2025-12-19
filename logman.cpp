// Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3251350
#include "logman.hpp"
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iterator>
using namespace std;

void Logman::getOptions(int argc, char **argv) { 
    int choice = 0;
    int index = 0;
    option longOptions[] = {
        {"help", no_argument, nullptr, 'h'},
        {"file", required_argument, nullptr, 'f'},
        {nullptr, 0, nullptr, '\0'},
    }; 

    while ((choice = getopt_long(argc, argv, "hf:", static_cast<option *>(longOptions), &index)) != -1) {
        switch (choice) {
        case 'h':
            printHelp(*argv);
            exit(0);
        case 'f': {
            string arg { optarg };
            master_log_file = arg;
            break;
        }
        default:
            cerr << "Unknown command line option\n";
            exit(1);
        } 
    }
}

void Logman::printHelp(char *command) {
    cout << "Usage: " << command << " [--help -h] [--file -f]\n"
         << "This program will begin by reading an input file containing log entries,\n"
         << "and then will enter an interactive mode where you can perform\n"
         << "timestamp, category, and keyword searches for the purpose of constructing an excerpt list of the log file.\n"
         << "You can also manage and display this excerpt list to identify the important/relevant entries of your log file.\n"
         << "The only possible commands are help or file. File must be followed by an argument,\n" 
         << "which should be the name of the log file you wish to read from.\n";
}

// takes the string version of the timestamp and erases all instances of : in the ts
// then uses stoull to convert to uint64_t. The purpose of this function is to allow for faster comparisons
uint64_t Logman::convert_to_num(string & ts) { 
    char c = ':';
    ts.erase(remove(ts.begin(), ts.end(), c), ts.end());
    return stoull(ts);
}

void Logman::readFile() {
    ifstream inputFile(master_log_file);
    string ts;
    string cat;
    string msg;
    while(getline(inputFile, ts, '|')) {
        getline(inputFile, cat, '|');
        getline(inputFile, msg);

        Entry e;
        e.ts = ts;
        e.ts_num = convert_to_num(ts);
        e.cat = cat;
        e.msg = msg;
        transform(cat.begin(), cat.end(), cat.begin(), ::tolower);
        e.lower_cat = cat;
        e.id = id_tracker++;
        entries.push_back(e); // building master log
    }
    cout << id_tracker << " entries read\n";
    sorted.reserve(entries.size()); // building sorted entryIDs vector
    for(size_t i = 0; i < entries.size(); i++) {
        sorted.push_back(static_cast<int>(i));
    }
    sort(sorted.begin(), sorted.end(), [&](size_t x, size_t y) { // this lambda allows for proper sorting logic + tiebreak logic as outlined by spec
        const Entry & a = entries[x];
        const Entry & b = entries[y];
        if(a.ts_num != b.ts_num) {
            return a.ts_num < b.ts_num;
        } else if (a.lower_cat != b.lower_cat) {
            return a.lower_cat < b.lower_cat;
        } else {
            return a.id < b.id;
        }
    });
    for (int id : sorted) { // building category map
        Entry & e = entries[static_cast<size_t>(id)];
        category_results[e.lower_cat].push_back(e.id); 
    }
    for(int id : sorted) { // building keyword map
        Entry & e = entries[static_cast<size_t>(id)];
        string combined = e.cat + " " + e.msg; // keyword looks at both category and message so I concatenate to look at both cat and msg at once
        transform(combined.begin(), combined.end(), combined.begin(), ::tolower);
        add_to_map(combined, e.id);
    }
    // records the an entry id's position from sorted vector. Entry ID x is at position curr_id_pos[x] in sorted vector
    curr_id_pos.resize(entries.size());
    for(size_t i = 0; i < sorted.size(); ++i) {
        curr_id_pos[static_cast<size_t>(sorted[i])] = static_cast<int>(i);
    }
    excerpt_list.reserve(entries.size());
}

// breaks up a string into just the words
void Logman::add_to_map(string & s, int id) {
    string curr; // curr is consistently emptied and recreated whenever we need to form a new word (from s)
    for(char c : s) {
        if(isalnum(c)) {
            curr += c;
        } else if (!curr.empty() && (keyword_results[curr].empty() || keyword_results[curr].back() != id)) { // ensures no duplicates. Only checking the back is necessary since values are sorted
            keyword_results[curr].push_back(id);
            curr = ""; // preparing curr for new word
        } else {
            curr = "";  
        }
    }
    // this function replies on nonalphanumerics to know when to start constructing a new word. This extra case is necessary if the string ends with a alphanum
    if (!curr.empty() && (keyword_results[curr].empty() || keyword_results[curr].back() != id)) {
        keyword_results[curr].push_back(id);
    }
}

void Logman::readUserInput() {
    char cmd;
    string junk;
    do {
        cout << "% ";
        cin >> cmd;
        if(cin.fail()) { // from spec
            cerr << "cin entered fail state: exiting\n";
            exit(1); 
        }
        switch (cmd) {
            case '#':
                getline(cin, junk);
                break;
            case 't': {
                cin >> ws;
                string t1;
                string t2;
                getline(cin, t1, '|');
                getline(cin, t2);
                if(t1.size() == 14 && t2.size() == 14) {
                    t_command(t1, t2);
                } else {
                    cerr << "Error: timestamps must be 14 characters\n";
                }
                break;
            }
            case 'm': {
                string t;
                cin >> ws;
                getline(cin, t);
                if(t.size() == 14) {
                    m_command(t);
                } else {
                    cerr << "Error: timestamps must be 14 characters\n";
                }
                break;
            }
            case 'c': {
                string c;
                cin >> ws;
                getline(cin, c); 
                c_command(c);
                break;
            }
            case 'k': {
                cin >> ws;
                string k;
                getline(cin, k);
                k_command(k);
                break;
            }
            case 'a': {
                string n;
                cin >> ws;
                getline(cin, n);
                a_command(stoi(n));
                break;
            }
            case 'r':
                r_command();
                break;
            case 'd': {
                string pos;
                cin >> ws;
                getline(cin, pos);
                d_command(stoi(pos));
                break;
            }
            case 'b': {
                string pos;
                cin >> ws;
                getline(cin, pos);
                b_command(stoi(pos));
                break;
            }
            case 'e': {
                string pos;
                cin >> ws;
                getline(cin, pos);
                e_command(stoi(pos));
                break;
            }
            case 's':
                s_command();
                break;
            case 'l':
                l_command();
                break;
            case 'g':
                g_command();
                break;
            case 'p':
                p_command();
                break;
            case 'q':
                break;
            default:
                getline(cin, junk);
                cerr << "Unknown command\n";
                break;
        }
    } while (cmd != 'q');
}

void Logman::a_command(int x) {
    if(x >= 0 && x < static_cast<int>(entries.size())) {
        excerpt_list.push_back(x);
        cout << "log entry " << x << " appended\n";
    }
}

void Logman::p_command() {
    for(size_t i = 0; i < excerpt_list.size(); i++) {
        int id = excerpt_list[i];
        Entry & e = entries[static_cast<size_t>(id)];
        cout << i << "|" << id << "|" << e.ts << "|" << e.cat << "|" << e.msg << "\n";
    }
}

void Logman::t_command(string & l, string & u) {
    uint64_t L = convert_to_num(l);
    uint64_t U = convert_to_num(u);
    auto lo = lower_bound(sorted.begin(), sorted.end(), L, [&] (int id, const uint64_t & val) {
        return entries[static_cast<size_t>(id)].ts_num < val;
    });
    auto hi = upper_bound(sorted.begin(), sorted.end(), U, [&] (const uint64_t & val, int id) {
        return val < entries[static_cast<size_t>(id)].ts_num;
    });
    last_search = 't';
    prev_search_exists = true;
    last_lo = lo;
    last_hi = hi;
    cout << "Timestamps search: " << hi - lo << " entries found\n";
}

void Logman::m_command(string & ts) {
    uint64_t T = convert_to_num(ts);
    auto lo = lower_bound(sorted.begin(), sorted.end(), T, [&] (int id, const uint64_t & val) {
        return entries[static_cast<size_t>(id)].ts_num < val;
    });
    auto hi = upper_bound(sorted.begin(), sorted.end(), T, [&] (const uint64_t & val, int id) {
        return val < entries[static_cast<size_t>(id)].ts_num;
    });
    last_search = 'm';
    prev_search_exists = true;
    last_lo = lo;
    last_hi = hi;
    cout << "Timestamp search: " << hi - lo << " entries found\n";
}

void Logman::c_command(string & category) {
    transform(category.begin(), category.end(), category.begin(), ::tolower);
    last_cat_vec = nullptr;
    auto it = category_results.find(category);
    size_t count = 0;
    if(it != category_results.end()) { // ensure the category exists
        count = it->second.size();
        last_cat_vec = &it->second;
    }
    last_search = 'c';
    prev_search_exists = true;
    cout << "Category search: " << count << " entries found\n";
}

void Logman::d_command(int pos) {
    if(pos >= 0 && pos < static_cast<int>(excerpt_list.size())) {
        excerpt_list.erase(excerpt_list.begin() + pos);
        cout << "Deleted excerpt list entry " << pos << "\n";
    }
}

void Logman::b_command(int pos) {
    if(pos >= 0 && pos < static_cast<int>(excerpt_list.size())) {
        // start at beginning, pos becomes the start of the rotated range (move pos to beginning), shift begin to pos down, maintaining order
        rotate(excerpt_list.begin(), excerpt_list.begin() + pos, excerpt_list.begin() + pos + 1);
        cout << "Moved excerpt list entry " << pos << "\n";
    }
}

void Logman::e_command(int pos) {
    if(pos >= 0 && pos < static_cast<int>(excerpt_list.size())) {
        // start as pos, take the range after pos until end, pos goes to the back, rest is preserved
        rotate(excerpt_list.begin() + pos, excerpt_list.begin() + pos + 1, excerpt_list.end());
        cout << "Moved excerpt list entry " << pos << "\n";
    }
}

void Logman::l_command() {
    size_t len = excerpt_list.size();
    cout << "excerpt list cleared\n";
    if(len != 0) {
        int first_id = excerpt_list.front();
        int last_id = excerpt_list.back();
        Entry & e = entries[static_cast<size_t>(first_id)];
        Entry & e2 = entries[static_cast<size_t>(last_id)];
        cout << "previous contents:\n";
        cout << "0|" << first_id << "|" << e.ts << "|" << e.cat << "|" << e.msg << "\n";
        cout << "...\n";
        cout << len - 1 << "|" << last_id << "|" << e2.ts << "|" << e2.cat << "|" << e2.msg << "\n";
    } else {
        cout << "(previously empty)\n";
    }
    excerpt_list.clear();
}

void Logman::s_command() {
    size_t len = excerpt_list.size();
    cout << "excerpt list sorted\n";
    if(len == 0) {
        cout << "(previously empty)\n";
        return;
    }
    int prev_first_id = excerpt_list.front();
    int prev_last_id = excerpt_list.back();
    Entry & e = entries[static_cast<size_t>(prev_first_id)];
    Entry & e2 = entries[static_cast<size_t>(prev_last_id)];
    cout << "previous ordering:\n";
    cout << "0|" << prev_first_id << "|" << e.ts << "|" << e.cat << "|" << e.msg << "\n";
    cout << "...\n";
    cout << len - 1 << "|" << prev_last_id << "|" << e2.ts << "|" << e2.cat << "|" << e2.msg << "\n";
    
    auto cmp = [&](int a, int b) {
        return curr_id_pos[static_cast<size_t>(a)] < curr_id_pos[static_cast<size_t>(b)];
    };

    sort(excerpt_list.begin(), excerpt_list.end(), cmp);

    int new_first_id = excerpt_list.front();
    int new_last_id = excerpt_list.back();
    Entry & e3 = entries[static_cast<size_t>(new_first_id)];
    Entry & e4 = entries[static_cast<size_t>(new_last_id)];
    cout << "new ordering:\n";
    cout << "0|" << new_first_id << "|" << e3.ts << "|" << e3.cat << "|" << e3.msg << "\n";
    cout << "...\n";
    cout << len - 1 << "|" << new_last_id << "|" << e4.ts << "|" << e4.cat << "|" << e4.msg << "\n";
}

void Logman::g_command() {
    if(!prev_search_exists) {
        cerr << "Error: no previous search\n";
        return;
    }
    if(last_search == 't' || last_search == 'm') {
        for(auto it = last_lo; it != last_hi; ++it) {
            int id = *it;
            const Entry & e = entries[static_cast<size_t>(id)];
            cout << e.id << "|" << e.ts << "|" << e.cat << "|" << e.msg << "\n";
        }
    }
    if(last_search == 'c') {
        if(last_cat_vec) {
            for(int id : *last_cat_vec) {
                const Entry & e = entries[static_cast<size_t>(id)];
                cout << e.id << "|" << e.ts << "|" << e.cat << "|" << e.msg << "\n";
            } 
        }
    }
    if(last_search == 'k') {
        for(int id : final) {
            const Entry & e = entries[static_cast<size_t>(id)];
            cout << e.id << "|" << e.ts << "|" << e.cat << "|" << e.msg << "\n";
        } 
    }
}

// same logic as g but it just appends
void Logman::r_command() {
    if(!prev_search_exists) {
        cerr << "Error: no previous search\n";
        return;
    }
    if(last_search == 't' || last_search == 'm') {
        for(auto it = last_lo; it != last_hi; ++it) {
            excerpt_list.push_back(*it);
        }
        cout << last_hi - last_lo << " log entries appended\n";
        return;
    }
    if(last_search == 'c') {
        size_t appended = 0;
        if(last_cat_vec) {
            for(int id : *last_cat_vec) {
                excerpt_list.push_back(id);
            } 
            appended = last_cat_vec->size();
        }
        cout << appended << " log entries appended\n";
        return;
    }
    if(last_search == 'k') {
        for(int id : final) {
            excerpt_list.push_back(id);
        } 
        cout << final.size() << " log entries appended\n";
        return;
    }
} 

void Logman::k_command(string & key) {
    vector<string> words; // all of the words from the string that was received from user k command
    vector<int> temp;
    string curr;
    for(char c : key) { // breaks down string, same logic as add to map function
        if(isalnum(c)) {
            curr.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
        } else if (!curr.empty()) {
            words.push_back(curr);
            curr = "";
        }
    }
    if(!curr.empty()) {
        words.push_back(curr);
    }

    last_search = 'k';
    prev_search_exists = true;

    if(words.empty()) {
        final.clear();
        cout << "Keyword search: 0 entries found\n";
        return;
    }

    auto cmp = [&](int a, int b) {
        return curr_id_pos[static_cast<size_t>(a)] < curr_id_pos[static_cast<size_t>(b)];
    };

    auto it = keyword_results.find(words[0]); 
    if(it == keyword_results.end()) {
        final.clear();
        cout << "Keyword search: 0 entries found\n";
        return;
    }
    final = it->second; // if the first word exists in the keyword map, this saves the value of that word key in final

    // the logic follows this pattern: start with final as words[0] vector. Each time, compare final with next word vector, save in temp. Swap temp with final so you are comparing new word vector with most recent set intersection each time
    for(size_t i = 1; i < words.size() && !final.empty(); ++i) {
        auto it2 = keyword_results.find(words[i]);
        if(it2 == keyword_results.end()) { // checks vector value for every word key with final
            final.clear();
            break;
        }
        temp.clear();
        temp.reserve(min(final.size(), it2->second.size())); // temp is the set intersection, so its size is guaranteed by final and it->second
        set_intersection(final.begin(), final.end(), it2->second.begin(), it2->second.end(), back_inserter(temp), cmp); // save results in temp
        final.swap(temp); // swap in temp, this is just for printing purposes in g and r
    }
    cout << "Keyword search: " << final.size() << " entries found\n";
}



