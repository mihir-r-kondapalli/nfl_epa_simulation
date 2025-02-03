#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <random> 
#include <chrono>
#include <sstream>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

const int SEED_VALUE = 25;
const vector<string> play_types = {"rush", "pass"};  // Play types

// Yardline bins (as defined in your new structure)
const vector<string> yardline_bins = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
    "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
    "21-23", "24-27", "28-32", "33-38", "39-44", "45-50", "51-70", "71-85", "86-99"
};

// Field Goal Probabilities by yardline position
const vector<double> fg_prob = {
    1.0, 0.9875, 1.0, 0.9919, 0.9937, 0.9929, 0.9797, 0.9818, 0.9693, 0.977, 
    0.9479, 0.983, 0.9777, 0.9459, 0.9659, 0.899, 0.92, 0.9167, 0.9347, 0.893, 
    0.9053, 0.8603, 0.7956, 0.822, 0.7885, 0.8, 0.7405, 0.7267, 0.7231, 0.6986,
    0.7394, 0.7416, 0.7216, 0.6911, 0.6994, 0.7059, 0.6129, 0.5595, 0.6271, 0.5833,
    0.5714, 0.4667, 0.4167, 0.5556, 0.375, 0.3333, 0.2, 0.05, 0.01, 0.005,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};


// Function to generate mapping from yardline (1-99) to index of yardline_bins
void generateYardlineMapping(vector<int>& yardline_mapping) {
    yardline_mapping.resize(100, -1); // Initialize with -1 for safety

    for (size_t index = 0; index < yardline_bins.size(); index++) {
        string bin = yardline_bins[index];

        // Handle individual yardline bins (e.g., "1", "2", ...)
        if (bin.find('-') == string::npos) {
            int yardline = stoi(bin);
            yardline_mapping[yardline] = index;
        } 
        // Handle grouped bins (e.g., "21-23", "24-27", ...)
        else {
            stringstream ss(bin);
            int start, end;
            char dash;
            ss >> start >> dash >> end;
            
            for (int yardline = start; yardline <= end; yardline++) {
                yardline_mapping[yardline] = index;
            }
        }
    }
}

// Function to generate filenames dynamically
vector<string> generateFilenames() {
    vector<string> filenames;
    for (const auto& play_type : play_types) {
        for (const auto& bin : yardline_bins) {
            filenames.push_back("sample_data/" + play_type + "_sample_yl" + bin + ".json");
        }
    }
    return filenames;
}

// Function to load JSON sample data into an unordered_map
void loadSampleData(const string& filename, unordered_map<string, vector<int>>& sample_data) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    json jsonData;
    file >> jsonData;

    for (auto& [key, values] : jsonData.items()) {
        vector<int> samples = values.get<vector<int>>();
        sample_data[key] = move(samples);
    }

    cout << "Loaded " << sample_data.size() << " keys from " << filename << endl;
}

// Struct for hashing pairs (used for unordered_map keys)
struct pair_hash {
    size_t operator()(const pair<string, int>& p) const {
        return hash<string>()(p.first) ^ hash<int>()(p.second);
    }
};

// Function to save results to CSV with separate Down and Distance columns
void saveDataToCSV(string filename, unordered_map<pair<string, int>, double, pair_hash>& data) {
    ofstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    // Write CSV Header
    file << "Down,Distance,Yardline,EPA\n";

    // Iterate through the data map
    for (const auto& [key, value] : data) {
        string down_distance = key.first;
        int yardline = key.second;
        int down, distance;

        // Parse the "Down-Distance" string into integers
        size_t dash_pos = down_distance.find('-');
        if (dash_pos != string::npos) {
            down = stoi(down_distance.substr(0, dash_pos));
            distance = stoi(down_distance.substr(dash_pos + 1));
        } else {
            cerr << "Error parsing Down-Distance format: " << down_distance << endl;
            continue;  // Skip if formatting is incorrect
        }

        // Write row to CSV
        file << down << "," << distance << "," << yardline << "," << value << "\n";
    }

    file.close();
    cout << "CSV file saved successfully: " << filename << endl;
}

// Function to save results to CSV with separate Down and Distance columns
void saveDataToCSV_opt(string filename, unordered_map<pair<string, int>, int, pair_hash>& data) {
    ofstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    // Write CSV Header
    file << "Down,Distance,Yardline,Optimal Choice\n";

    // Iterate through the data map
    for (const auto& [key, value] : data) {
        string down_distance = key.first;
        int yardline = key.second;
        int down, distance;

        // Parse the "Down-Distance" string into integers
        size_t dash_pos = down_distance.find('-');
        if (dash_pos != string::npos) {
            down = stoi(down_distance.substr(0, dash_pos));
            distance = stoi(down_distance.substr(dash_pos + 1));
        } else {
            cerr << "Error parsing Down-Distance format: " << down_distance << endl;
            continue;  // Skip if formatting is incorrect
        }

        // Write row to CSV
        file << down << "," << distance << "," << yardline << "," << value << "\n";
    }

    file.close();
    cout << "CSV file saved successfully: " << filename << endl;
}


// Number of simulations for each down, distance, and yardline
const int N = 100;

const double TD_VAL = 6.945;
const double FG_VAL = 3;
const double KO_VAL = 0;   // Average EP of kickoff for opponent

unordered_map<pair<string, int>, double, pair_hash> run_epas;
unordered_map<pair<string, int>, double, pair_hash> pass_epas;
unordered_map<pair<string, int>, double, pair_hash> kick_epas;
unordered_map<pair<string, int>, double, pair_hash> max_epas;
unordered_map<pair<string, int>, int, pair_hash> opt_choices;

vector<double> probs(100);
vector<pair<int, int>> distrs(100);

// Function to sample from the JSON data
int get_val(string down_and_distance, unordered_map<string, vector<int>>& sample_data, mt19937& rng, int yardline) {
    vector<int> sample = sample_data[down_and_distance];
    if (!sample.empty()) {
        return sample[rng() % sample.size()];
    }

    if (down_and_distance[0] != '1') {
        down_and_distance[0]--;
        return get_val(down_and_distance, sample_data, rng, yardline);
    }

    if(yardline < 10){
        return get_val("1-"+to_string(yardline), sample_data, rng, yardline);
    }
    return get_val("1-10", sample_data, rng, yardline);
}

// Function to compute EPA values based on sampled results
double get_epa_val(int val, int down, int yards_to_go, int yardline) {
    if (val < -100) {
        return 0;
    }

    int new_yardline = yardline - val;
    if (new_yardline <= 0) {
        return TD_VAL - KO_VAL; // Touchdown + Expected Extra Point
    }

    if (new_yardline >= 100) {
        return -2;  // Safety placeholder
    }

    int new_down = down + 1;
    int new_yards_to_go = yards_to_go - val;

    if (new_yards_to_go <= 0) {
        new_down = 1;
        new_yards_to_go = (10 <= yardline) ? 10 : yardline;
    } else if (new_yards_to_go > 0 && new_down > 4) {
        return 0;
    }

    string new_down_and_distance = to_string(new_down) + "-" + to_string(yards_to_go);
    return max_epas[make_pair(new_down_and_distance, new_yardline)];
}

// Run the simulation
void run_simulation(vector<unordered_map<string, vector<int>>>& sample_datas, mt19937& mt1, vector<int>& yardline_mapping) {
    uniform_real_distribution<double> dist(0.0, 1.0);

    int yardline;
    int down;
    int yards_to_go;

    for (yardline = 1; yardline < 100; yardline++) {
        for (down = 4; down > 0; down--) {
            for (yards_to_go = 1; yards_to_go <= 20; yards_to_go++) {
                if (yards_to_go > yardline) continue;

                string down_and_distance = to_string(down) + "-" + to_string(yards_to_go);
                double epa_rush_val = 0;
                double epa_pass_val = 0;
                double epa_kick_val = (down == 4) ? fg_prob[yardline-1]*FG_VAL : -1000;  // only viable if it is 4th down

                int n = (down == 1 && yards_to_go != 10 && yards_to_go != yardline) ? 50 : N;

                for (int i = 0; i < n; i++) {
                    int sample_num;
                    // int sample_num = (dist(mt1) < probs[yardline - 1]) ? distrs[yardline - 1].second : distrs[yardline - 1].first;
                    sample_num = yardline_mapping[yardline];
                    epa_rush_val += get_epa_val(get_val(down_and_distance, sample_datas[sample_num], mt1, yardline), down, yards_to_go, yardline);
                    epa_pass_val += get_epa_val(get_val(down_and_distance, sample_datas[yardline_bins.size()+sample_num], mt1, yardline), down, yards_to_go, yardline);
                }

                epa_rush_val /= n;
                epa_pass_val /= n;

                pair<string, int> ddy = make_pair(down_and_distance, yardline);

                run_epas[ddy] = epa_rush_val;
                pass_epas[ddy] = epa_pass_val;
                kick_epas[ddy] = epa_kick_val;
                max_epas[ddy] = max({epa_rush_val, epa_pass_val, epa_kick_val});

                int max_index = 0; // Assume epa_rush_val is max
                if (epa_pass_val > epa_rush_val) max_index = 1;
                if (epa_kick_val > (max_index == 0 ? epa_rush_val : epa_pass_val)) max_index = 2;

                opt_choices[ddy] = max_index;

                cout << down_and_distance << " " << yardline << ": " << epa_rush_val << ", " << epa_pass_val << ", " << epa_kick_val << endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    vector<string> filenames = generateFilenames();  // Generate filenames dynamically
    vector<unordered_map<string, vector<int>>> sample_datas(filenames.size());

    int seed_value = (argc == 2) ? stoi(argv[1]) : SEED_VALUE;
    mt19937 mt1(seed_value);

    for (size_t i = 0; i < filenames.size(); i++) {
        loadSampleData(filenames[i], sample_datas[i]);
    }

    cout << "Data loaded successfully!" << endl;

    vector<int> yardline_mapping;
    generateYardlineMapping(yardline_mapping);

    cout << "Yardline Mapping Generated!" << endl;

    auto start = chrono::high_resolution_clock::now();
    run_simulation(sample_datas, mt1, yardline_mapping);
    saveDataToCSV("naive_kick_epas/run_epas.csv", run_epas);
    saveDataToCSV("naive_kick_epas/pass_epas.csv", pass_epas);
    saveDataToCSV("naive_kick_epas/kick_epas.csv", kick_epas);
    saveDataToCSV("naive_kick_epas/max_epas.csv", max_epas);
    saveDataToCSV_opt("naive_kick_epas/opt_choices.csv", opt_choices);

    auto end = chrono::high_resolution_clock::now();
    cout << "Execution time: " << chrono::duration<double>(end - start).count() << " seconds" << endl;

    return 0;
}
