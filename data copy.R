# Load necessary libraries
options(repos = c(CRAN = "https://cran.rstudio.com/"))
library(nflfastR)
library(tidyverse)
library(jsonlite)

# Load play-by-play data for the 2023 and 2024 seasons
pbp_data <- load_pbp(c(2018, 2019, 2021, 2022, 2023, 2024))

# Define yardline bins
# yardline_bins <- list("1-10" = 1:10, "11-20" = 11:20, "21-99" = 21:99)

# Generate individual bins for yardlines 1-20
individual_bins <- setNames(as.list(1:20), as.character(1:20))

# Define grouped bins for higher yardlines
grouped_bins <- list(
  "21-23"  = 21:23,
  "24-27"  = 24:27,
  "28-32"  = 28:32,
  "33-38"  = 33:38,
  "39-44"  = 39:44,
  "45-50"  = 45:50,
  "51-70"  = 51:70,
  "71-85"  = 71:85,
  "86-99"  = 86:99
)

# Combine both individual and grouped bins
yardline_bins <- c(individual_bins, grouped_bins)

# Loop through each yardline group
for (yardline_range in names(yardline_bins)) {
  lower_yl <- min(yardline_bins[[yardline_range]])
  upper_yl <- max(yardline_bins[[yardline_range]])

  # Initialize lookup tables
  rush_distributions <- list()
  pass_distributions <- list()

  # Loop through all down & distance combinations
  for (input_down in 1:4) {
    for (yards in 1:20) {
      key <- paste(input_down, yards, sep = "-")  # Create a unique key

      # Extract real rush data
      rush_yards <- pbp_data %>%
        filter(play_type == "run", qb_scramble == 0, down == input_down, ydstogo == yards, yardline_100 >= lower_yl, yardline_100 <= upper_yl) %>%
        mutate(yards_gained = ifelse(fumble_lost == 1, -1100 - return_yards + yards_gained, yards_gained)) %>%
        mutate(yards_gained = ifelse(rush_touchdown == 1,  10 + yards_gained, yards_gained)) %>%
        pull(yards_gained)

      # Extract real pass data
      pass_yards <- pbp_data %>%
        filter((play_type == "pass" | qb_scramble == 1), down == input_down, ydstogo == yards, yardline_100 >= lower_yl, yardline_100 <= upper_yl) %>%
        mutate(yards_gained = ifelse(fumble_lost == 1, -1100 - return_yards + yards_gained, yards_gained)) %>%
        mutate(yards_gained = ifelse(interception == 1, -2100 - return_yards + air_yards, yards_gained)) %>%
        mutate(yards_gained = ifelse(pass_touchdown == 1,  10 + yards_gained, yards_gained)) %>%
        mutate(yards_gained = ifelse(rush_touchdown == 1,  10 + yards_gained, yards_gained)) %>%
        pull(yards_gained)

      # Store in lookup table (as vectors)
      rush_distributions[[key]] <- rush_yards
      pass_distributions[[key]] <- pass_yards

      print(paste(yardline_range, ": ", (input_down-1)*20+yards))
    }
  }

  # Save the lookup tables as JSON
  rush_filename <- paste0("distr_data/rush_distributions_yl", yardline_range, ".json")
  pass_filename <- paste0("distr_data/pass_distributions_yl", yardline_range, ".json")
  
  write_json(rush_distributions, rush_filename, pretty = TRUE, auto_unbox = TRUE)
  write_json(pass_distributions, pass_filename, pretty = TRUE, auto_unbox = TRUE)
}

print("Lookup tables saved as JSON for each yardline group!")
