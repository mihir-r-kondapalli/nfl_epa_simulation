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
vector<string> generateFilenames(string dir_name) {
    vector<string> filenames;
    for (const auto& play_type : play_types) {
        for (const auto& bin : yardline_bins) {
            filenames.push_back(dir_name + "/" + play_type + "_cdf_yl" + bin + ".json");
        }
    }
    return filenames;
}

// Structure to hold a CDF entry
struct CDFEntry {
    vector<int> values;
    vector<double> cdf;
};

// Function to load CDF-based JSON into map<string, CDFEntry>
void loadCDFData(const string& filename, unordered_map<string, CDFEntry>& sample_data) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    json jsonData;
    file >> jsonData;

    for (auto& [key, value] : jsonData.items()) {
        CDFEntry entry;
        try {
            entry.values = value["values"].get<vector<int>>();
            entry.cdf = value["cdf"].get<vector<double>>();
        }
        catch (const exception& e){
            entry.values = vector<int>();
            entry.values.push_back(value["values"].get<int>());
            entry.cdf = vector<double>();
            entry.cdf.push_back(value["cdf"].get<double>());
        }
        sample_data[key] = move(entry);
    }

    cout << "Loaded CDF data from " << filename << ", " << sample_data.size() << " keys." << endl;
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
    file << "Down,Distance,Yardline,EP\n";

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
void saveDataToCSV(string filename, unordered_map<pair<string, int>, double, pair_hash>& run_data,
                unordered_map<pair<string, int>, double, pair_hash>& pass_data,
                unordered_map<pair<string, int>, double, pair_hash>& kick_data,
                unordered_map<pair<string, int>, double, pair_hash>& max_data,
                unordered_map<pair<string, int>, int, pair_hash>& opt_data) {
    ofstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    // Write CSV Header
    file << "Down,Distance,Yardline,Run_EP,Pass_EP,Kick_EP,Punt_EP,EP,Opt_Choice\n";

    // Use run_data as the main loop (should have all keys)
    for (const auto& [key, run_val] : run_data) {
        string down_distance = key.first;
        int yardline = key.second;
        int down, distance;

        size_t dash_pos = down_distance.find('-');
        if (dash_pos == string::npos) {
            cerr << "Error parsing Down-Distance format: " << down_distance << endl;
            continue;
        }

        down = stoi(down_distance.substr(0, dash_pos));
        distance = stoi(down_distance.substr(dash_pos + 1));

        double pass_val = pass_data[key];
        double kick_val = kick_data[key];
        double punt_val = 0;
        double max_val = max_data[key];
        double opt_choice = opt_data[key];

        file << down << "," << distance << "," << yardline << ","
        << run_val << "," << pass_val << "," << kick_val << "," 
        << punt_val << "," << max_val << "," << opt_choice << "\n";
    }

    file.close();
    cout << "Combined CSV saved to: " << filename << endl;
}

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
        new_yards_to_go = (10 <= new_yardline) ? 10 : new_yardline;
    } else if (new_yards_to_go > 0 && new_down > 4) {
        return 0;
    }

    string new_down_and_distance = to_string(new_down) + "-" + to_string(new_yards_to_go);
    return max_epas[make_pair(new_down_and_distance, new_yardline)];
}

// Run the simulation
void run_simulation(vector<unordered_map<string, CDFEntry>>& cdf_datas, vector<int>& yardline_mapping) {
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
                pair<string, int> ddy = make_pair(down_and_distance, yardline);

                int sample_num;
                sample_num = yardline_mapping[yardline];

                CDFEntry cdf;

                epa_rush_val = 0;
                epa_pass_val = 0;

                cdf = cdf_datas[sample_num][down_and_distance];
                double prev = 0.0;
                for (int i = 0; i < cdf.values.size(); i++) {
                    epa_rush_val += (cdf.cdf[i]-prev) * get_epa_val(cdf.values[i], down, yards_to_go, yardline);
                    prev = cdf.cdf[i];
                }

                cdf = cdf_datas[yardline_bins.size()+sample_num][down_and_distance];
                prev = 0.0;
                for (int i = 0; i < cdf.values.size(); i++) {
                    epa_pass_val += (cdf.cdf[i]-prev) * get_epa_val(cdf.values[i], down, yards_to_go, yardline);
                    prev = cdf.cdf[i];
                }

                if (epa_rush_val > 1e10 || epa_pass_val > 1e10 || epa_rush_val<=-1e6) {
                    cerr << down_and_distance << " " << yardline << ": Large EPAs encounted (Re-run program): " << epa_rush_val << ", " << epa_pass_val << endl;
                    exit(1);
                }

                run_epas[ddy] = epa_rush_val;
                pass_epas[ddy] = epa_pass_val;
                kick_epas[ddy] = epa_kick_val;
                max_epas[ddy] = max({epa_rush_val, epa_pass_val, epa_kick_val});
                
                vector<double> epas = {epa_rush_val, epa_pass_val, epa_kick_val};
                int max_index = distance(epas.begin(), max_element(epas.begin(), epas.end()));
                opt_choices[ddy] = max_index;

                cout << down_and_distance << " " << yardline << ": " << epa_rush_val << ", " << epa_pass_val << ", " << epa_kick_val << endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {

    if(argc != 3){
        cout << "Need to provide target file and cdf directory (target_eps.csv cdf_data)" << endl;
        return -1;
    }

    string target_file = argv[1];
    string cdf_dir = argv[2];

    vector<string> filenames = generateFilenames(cdf_dir);  // Generate filenames dynamically
    vector<unordered_map<string, CDFEntry>> cdf_datas(filenames.size());

    for (size_t i = 0; i < filenames.size(); i++) {
        loadCDFData(filenames[i], cdf_datas[i]);
    }

    cout << "Data loaded successfully!" << endl;

    vector<int> yardline_mapping;
    generateYardlineMapping(yardline_mapping);

    cout << "Yardline Mapping Generated!" << endl;

    auto start = chrono::high_resolution_clock::now();
    run_simulation(cdf_datas, yardline_mapping);
    saveDataToCSV(target_file, run_epas, pass_epas, kick_epas, max_epas, opt_choices);

    auto end = chrono::high_resolution_clock::now();
    cout << "Execution time: " << chrono::duration<double>(end - start).count() << " seconds" << endl;

    return 0;
}
