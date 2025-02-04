# 🏈 NFL EPA Simulation

This repository contains a simulation model for calculating **Expected Points (EP) and Expected Points Added (EPA)** in American football. The model is designed to replicate and compare results against **NFLFastR's Objective EP model**, with improvements to better handle possession changes and field position adjustments.

## 📌 Features
- Simulates **Expected Points (EP)** for each yard line, down, and distance.
- Computes **Net EP** by considering possession changes.
- Compares results against **NFLFastR's EP model**.
- Supports different play types: **passes, runs, field goals, and punts**.
- Provides insights for **situational decision-making** and **play selection optimization**.

## Installation & Setup

### Clone the Repository
```sh
git clone https://github.com/mihir-r-kondapalli/nfl_epa_simulation.git
cd nfl_epa_simulation
```

### Install Dependencies
Ensure you have **R and nflfastR** installed.  
For Python simulations, install dependencies with:
```sh
pip install numpy pandas matplotlib
```
For R:
```r
install.packages("nflfastR")
install.packages("tidyverse")
```

## 🚀 Usage

### Running the C++ Simulation
```sh
./simulator_naive.out naive_eps {random seed, ex: 14}        # To get naive ep values
./simulator.out naive_eps first_eps {random seed, ex: 14}    # To get propagated ep values
```

## Comparing with NFLFastR
To compare simulated **EP values** with **NFLFastR**, use:
```r
library(nflfastR)

pbp <- load_pbp(c(2018, 2019, 2021, 2022, 2023))
ep_values <- pbp %>%
  group_by(yardline_100) %>%
  summarize(avg_ep = mean(ep, na.rm = TRUE))
```
Then plot **model vs. NFLFastR’s data**.

## File Structure

- README.md           # This file
- simulator_naive.cpp # C++ script for simulating naive EP values (EP values with no prior knowledge)
- simulator.cpp       # C++ script for simulating EP (EP values with prior runs of simulator.cpp and simulator_naive.cpp as priors)
- data.R              # R script that scrapes play-by-play data from NFLFastR  (play-by-play data for a given down, distance, and yardline)
- sampler_direct.R    # R script that samples data directly from data.R play-by-play data
- nfl_pbp_data.csv    # Processed play-by-play NFL EP data
- optimal_play_types.csv # Best play decisions based on NFL EP
- data_vis.ipynb      # Plots that compare simulated EPs (discounting opponent possessions after scores) to NFL EPs   (this shows how accurate the simulation is)
- data_vis_unbiased.ipynb # Plots that compare simulated Net EPs to NFL EPs
- unbiased_eps and biased_eps # folders that store EP values for runs, passes, kicks, and punts given a certain down, distance, and yardline

## Notes & Considerations
- **NFLFastR’s EP model does NOT subtract opponent EP after a score**, which can overestimate drive values.
- **Net EP is better** for measuring true drive impact.  (plotted in data_vis_unbiased.ipynb and stored in unbiased_eps)
- EP values near a team's **own goal line** account for safeties and bad punts.

## Acknowledgments
- **NFLFastR team** for their extensive play-by-play data.
- **[@mihir-r-kondapalli](https://github.com/mihir-r-kondapalli)** for developing the EP simulation.

