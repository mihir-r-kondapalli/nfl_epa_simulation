#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <random> 
#include <chrono>
#include <sstream>
#include "json.hpp"
#include <cstdlib>

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
    0.7394, 0.7416, 0.7216, 0.6911, 0.6994, 0.7059, 0.6129, 0.5595, 0.6271, 0.5297,
    0.4935, 0.4548, 0.4136, 0.3697, 0.3230, 0.2735, 0.2211, 0.1655, 0.1069, 0.0450,
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


void loadPriorData(const string& filename, vector<double>& data) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    // Resize vector to store values for yardline 1 to 99
    data.assign(99, 0.0);  

    string line;
    getline(file, line);  // Skip header

    while (getline(file, line)) {
        stringstream ss(line);
        int down, distance, yardline, opt_choice;
        double run_ep, pass_ep, kick_ep, punt_ep, max_ep;
        char comma;
    
        ss >> down >> comma >> distance >> comma >> yardline >> comma
           >> run_ep >> comma >> pass_ep >> comma >> kick_ep >> comma
           >> punt_ep >> comma >> max_ep >> comma >> opt_choice;
    
        // Store the value only if down == 1 and the specified conditions are met
        if (down == 1 && ((distance == 10 && yardline >= 10) || (distance == yardline && yardline < 10))) {
            int index = yardline - 1;  // Convert 1-based yardline to 0-based index
            data[index] = max_ep;
        }
    }    
    

    file.close();
    cout << "Successfully loaded prior EP data for " 
         << count_if(data.begin(), data.end(), [](double v) { return v != 0.0; }) 
         << " yardlines." << endl;
}


void loadPuntNetYards(vector<vector<int>>& puntYards, const string& filename) {
    // Open JSON file
    ifstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    json jsonData;
    file >> jsonData; // Parse JSON
    file.close();

    // Resize vector to ensure it has 99 elements (index 0 = yardline 1)
    puntYards.resize(99);

    // Process JSON data
    for (auto& [key, value] : jsonData.items()) {
        int yardline = stoi(key);
        if (yardline >= 1 && yardline <= 99) {
            int index = yardline - 1; // Convert yardline to 0-based index
            vector<int> yards = value.get<vector<int>>(); // Extract values

            // If the yardline vector has fewer than 10 elements, make it empty
            if (yards.size() < 10) {
                yards.clear();
            }

            puntYards[index] = yards;
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

// Structure to hold a Decision entry
struct DECISION_ENTRY {
    int run;
    int pass;
    int kick;
    int punt;
};

void loadDecisionData(const string& filename, unordered_map<pair<string, int>, DECISION_ENTRY, pair_hash>& decision_data) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    string line;
    getline(file, line);  // Skip header

    int count = 0;

    while (getline(file, line)) {
        stringstream ss(line);
        int down, ydstogo, yardline;
        int run, pass, kick, punt;
        char comma;

        // Read integers using ss >> ... >> comma >> ...
        ss >> down >> comma
           >> ydstogo >> comma
           >> yardline >> comma
           >> run >> comma
           >> pass >> comma
           >> kick >> comma
           >> punt;

        // Build key from down and ydstogo as string, and yardline
        string down_and_distance = to_string(down) + "-" + to_string(ydstogo);
        pair<string, int> key = make_pair(down_and_distance, yardline);

        DECISION_ENTRY entry = {run, pass, kick, punt};
        decision_data[key] = entry;
        count++;
    }

    file.close();
    cout << "Successfully loaded decision data for " << count << " entries." << endl;
}

// Function to save results to CSV with separate Down and Distance columns
void saveDataToCSV(string filename, unordered_map<pair<string, int>, double, pair_hash>& run_data,
        unordered_map<pair<string, int>, double, pair_hash>& pass_data,
        unordered_map<pair<string, int>, double, pair_hash>& kick_data,
        unordered_map<pair<string, int>, double, pair_hash>& punt_data,
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
        double punt_val = punt_data[key];
        double max_val = max_data[key];
        double opt_choice = opt_data[key];

        file << down << "," << distance << "," << yardline << ","
            << run_val << "," << pass_val << "," << kick_val << "," << punt_val << "," << max_val << "," << opt_choice << "\n";
    }

    file.close();
    cout << "Combined CSV saved to: " << filename << endl;
}

const double TD_VAL = 6.945;
const double FG_VAL = 3;
double KO_VAL = 0;
double SKO_VAL = 0; // safety kickoff
double TB_VAL = 0;

unordered_map<pair<string, int>, double, pair_hash> run_epas;
unordered_map<pair<string, int>, double, pair_hash> pass_epas;
unordered_map<pair<string, int>, double, pair_hash> kick_epas;
unordered_map<pair<string, int>, double, pair_hash> punt_epas;
unordered_map<pair<string, int>, double, pair_hash> max_epas;
unordered_map<pair<string, int>, int, pair_hash> opt_choices;
unordered_map<pair<string, int>, bool, pair_hash> visited; // to prevent stack overflow
unordered_map<pair<string, int>, double, pair_hash> priorData; // to prevent stack overflow

void loadPriorDataFromCSV(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    string line;
    getline(file, line);  // Skip header

    int count = 0;

    while (getline(file, line)) {
        stringstream ss(line);
        int down, ydstogo, yardline;
        double run, pass, kick, punt, max;
        char comma;

        // Read integers using ss >> ... >> comma >> ...
        ss >> down >> comma
           >> ydstogo >> comma
           >> yardline >> comma
           >> run >> comma
           >> pass >> comma
           >> kick >> comma
           >> punt >> comma
           >> max;

        // Build key from down and ydstogo as string, and yardline
        string down_and_distance = to_string(down) + "-" + to_string(ydstogo);
        pair<string, int> key = make_pair(down_and_distance, yardline);

        priorData[key] = max;
        count++;
    }

    file.close();
    cout << "Successfully loaded prior data." << endl;
}

vector<double> probs(100);
vector<pair<int, int>> distrs(100);

vector<vector<int>> punt_data;
vector<double> prior_epas;

double get_epa(int down, int yards_to_go, int yardline,
    std::vector<std::unordered_map<std::string, CDFEntry>>& cdf_datas,
    std::vector<int>& yardline_mapping,
    std::unordered_map<std::pair<std::string, int>, DECISION_ENTRY, pair_hash>& decision_data);

double get_epa_kick_val(int yardline){
    double miss_penalty = (yardline+7 < 100) ? -(1-fg_prob[yardline-1])*prior_epas[100-(yardline+7)-1] : -2 - SKO_VAL;
    return fg_prob[yardline-1]*(FG_VAL - KO_VAL) + miss_penalty;
}

double get_epa_single_punt(int yardline, int index){
    int val = punt_data[yardline-1][index];
    if(val < -1000){
        return -TD_VAL;
    }
    if(val > 1000){
        int new_yardline = yardline-(val-1000);  // recovered muffed punt
        if(new_yardline <= 0){
            return TD_VAL - KO_VAL;
        }
        int yl = (new_yardline < 10) ? new_yardline : 10;
        return prior_epas[new_yardline-1];
    }

    if(yardline-val <= 0){
        return -TB_VAL;
    }

    if ((100-(yardline-val))-1 <= 0) return -TD_VAL; // Because of error in punt data
    return -prior_epas[(100-(yardline-val))-1];   // Other team gets ball
}

double get_epa_punt_val(int yardline){
    int num_punts = punt_data[yardline-1].size();
    if(num_punts == 0){
        return -TB_VAL;
    }
    double epa_val = 0.0;
    for(int i = 0; i<num_punts; i++){
        epa_val += get_epa_single_punt(yardline, i);
    }
    return epa_val/num_punts;
}

// Function to compute EPA values based on sampled results
double get_epa_for_val(int val, int down, int yards_to_go, int yardline, vector<unordered_map<string, CDFEntry>>& cdf_datas,
                vector<int>& yardline_mapping, unordered_map<pair<string, int>, DECISION_ENTRY, pair_hash>& decision_data) {
    
    if (val < -2000) {
        int new_yl = 100-(yardline-(val+2100));
        if(new_yl >= 100){
            return -TB_VAL;  // Interception touchback
        } else if(new_yl <= 0){
            return -TD_VAL;
        }
        return -prior_epas[new_yl-1];
    }

    if (val < -1000) {
        int new_yl = 100-(yardline-(val+1100));
        if(new_yl >= 100){
            return -TB_VAL;  // Interception touchback
        } else if(new_yl <= 0){
            return -TD_VAL;
        }
        return -prior_epas[new_yl-1];
    }

    int new_yardline = yardline - val;
    if (new_yardline <= 0) {
        return TD_VAL - KO_VAL; // Touchdown + Expected Extra Point - EP after kickoff
    }

    if (new_yardline >= 100) {
        return -2 - SKO_VAL;  // Safety - EP after kickoff  (new safety kick rules make it essentially the same as a regular kickoff)
    }

    int new_down = down + 1;
    int new_yards_to_go = yards_to_go - val;

    if (new_yards_to_go <= 0) {
        new_down = 1;
        new_yards_to_go = (10 <= new_yardline) ? 10 : new_yardline;
    } else if (new_yards_to_go > 0 && new_down > 4) {
        return -prior_epas[(100-new_yardline)-1];
    }

    if(new_yards_to_go > new_yardline){
        return 0;
    }

    if (new_yards_to_go > 20) new_yards_to_go = 20;

    string new_down_and_distance = to_string(new_down) + "-" + to_string(new_yards_to_go);
    pair<string, int> ddy = make_pair(new_down_and_distance, new_yardline);
    if(max_epas.find(ddy) == max_epas.end()){
        double epa = get_epa(new_down, new_yards_to_go, new_yardline, cdf_datas, yardline_mapping, decision_data);
        return epa;
    }
    
    return max_epas[ddy];
}

double get_epa(int down, int yards_to_go, int yardline, vector<unordered_map<string, CDFEntry>>& cdf_datas,
            vector<int>& yardline_mapping, unordered_map<pair<string, int>, DECISION_ENTRY, pair_hash>& decision_data){

    string down_and_distance = to_string(down) + "-" + to_string(yards_to_go);
    pair<string, int> ddy = make_pair(down_and_distance, yardline);

    if(visited[ddy]){
        //cout << "hi" << down << yards_to_go << yardline << endl;
        //cout << down_and_distance << " " << yardline << endl;
        //cout << priorData[ddy] << endl;
        return priorData[ddy];
    }
    visited[ddy] = true;

    double epa_rush_val = 0;
    double epa_pass_val = 0;
    double epa_kick_val = get_epa_kick_val(yardline);  // only viable if it is 4th down
    double epa_punt_val = get_epa_punt_val(yardline);

    int sample_num;
    // int sample_num = (dist(mt1) < probs[yardline - 1]) ? distrs[yardline - 1].second : distrs[yardline - 1].first;
    sample_num = yardline_mapping[yardline];
    CDFEntry cdf = cdf_datas[sample_num][down_and_distance];
    DECISION_ENTRY dec = decision_data[ddy];
    if (down != 4) {
        dec.kick = 0;
        dec.punt = 0;
    }    
    double sum = dec.run + dec.pass + dec.kick + dec.punt;
    if (sum == 0){
        sum = 1;
    }

    double prev = 0.0;
    for (int i = 0; i < cdf.values.size(); i++) {
        double epa = get_epa_for_val(cdf.values[i], down, yards_to_go, yardline, cdf_datas,
            yardline_mapping, decision_data);
        epa_rush_val += (cdf.cdf[i]-prev) * epa;
        prev = cdf.cdf[i];
    }
    cdf = cdf_datas[yardline_bins.size()+sample_num][down_and_distance];
    prev = 0.0;
    for (int i = 0; i < cdf.values.size(); i++) {
        double epa = get_epa_for_val(cdf.values[i], down, yards_to_go, yardline, cdf_datas, yardline_mapping, decision_data);
        epa_pass_val += (cdf.cdf[i]-prev) * epa;
        prev = cdf.cdf[i];
    }

    run_epas[ddy] = epa_rush_val;
    pass_epas[ddy] = epa_pass_val;
    kick_epas[ddy] = epa_kick_val;
    punt_epas[ddy] = epa_punt_val;
    max_epas[ddy] = dec.run/sum * epa_rush_val + dec.pass/sum * epa_pass_val + dec.kick/sum * epa_kick_val + dec.punt/sum * epa_punt_val;

    vector<double> epas = {epa_rush_val, epa_pass_val, epa_kick_val, epa_punt_val};
    int max_index = distance(epas.begin(), max_element(epas.begin(), epas.end()));
    opt_choices[ddy] = max_index;

    return max_epas[ddy];
}

// Run the simulation
void run_simulation(vector<unordered_map<string, CDFEntry>>& cdf_datas, vector<int>& yardline_mapping,
                    unordered_map<pair<string, int>, DECISION_ENTRY, pair_hash>& decision_data) {
    uniform_real_distribution<double> dist(0.0, 1.0);

    int yardline;
    int down;
    int yards_to_go;

    for (down = 4; down > 0; down--) {
        for (yardline = 1; yardline < 100; yardline++) {
            for (yards_to_go = 1; yards_to_go <= 20; yards_to_go++) {
                if (yards_to_go > yardline) continue;            // Can't have first and 10 from 5 yard line
                double epa = get_epa(down, yards_to_go, yardline, cdf_datas, yardline_mapping, decision_data);
                // cout << down << " " << yards_to_go << " " << yardline << " " << epa << endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {

    if(argc != 6){
        cout << "Need to input prior and target ep files, punt yard data, cdf data, and decision data file: " << 
                    "(./simulator_norm.out prior_eps.csv target_eps.csv punt_net_yards.json cdf_data, nfl_decisions.csv)" << endl;
        return 1;
    }

    string prior_file = argv[1];
    string target_file = argv[2];
    string punt_file = argv[3];
    string cdf_dir = argv[4];
    string dec_data = argv[5];

    vector<string> filenames = generateFilenames(cdf_dir);  // Generate filenames dynamically
    vector<unordered_map<string, CDFEntry>> cdf_datas(filenames.size());
    unordered_map<pair<string, int>, DECISION_ENTRY, pair_hash> decision_data;

    for (size_t i = 0; i < filenames.size(); i++) {
        loadCDFData(filenames[i], cdf_datas[i]);
    }

    loadDecisionData(dec_data, decision_data);
    loadPriorDataFromCSV(prior_file);

    cout << "Data loaded successfully!" << endl;

    vector<int> yardline_mapping;
    generateYardlineMapping(yardline_mapping);
    loadPuntNetYards(punt_data, punt_file);

    loadPriorData(prior_file, prior_epas);

    KO_VAL = 0; // prior_epas[70-1];  // 0 for biased (mimicing nflfastr ep calculations)
    SKO_VAL = prior_epas[70-1];
    TB_VAL = prior_epas[80-1];

    cout << "Yardline Mapping Generated!" << endl;

    auto start = chrono::high_resolution_clock::now();
    run_simulation(cdf_datas, yardline_mapping, decision_data);
    saveDataToCSV(target_file, run_epas, pass_epas, kick_epas, punt_epas, max_epas, opt_choices);

    auto end = chrono::high_resolution_clock::now();
    cout << "Execution time: " << chrono::duration<double>(end - start).count() << " seconds" << endl;

    return 0;
}
