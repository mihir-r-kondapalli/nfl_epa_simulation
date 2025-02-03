# Install required packages if not already installed
if (!requireNamespace("nflfastR", quietly = TRUE)) install.packages("nflfastR")
if (!requireNamespace("jsonlite", quietly = TRUE)) install.packages("jsonlite")

# Load required libraries
library(nflfastR)
library(dplyr)
library(jsonlite)

# Load the play-by-play data (replace 2023 with the desired season)
pbp <- load_pbp(c(2018, 2019, 2021, 2022, 2023, 2024))

colnames(pbp)

# Filter for punt plays
punt_data <- pbp %>%
  filter(play_type == "punt") %>%
  select(yardline_100, kick_distance, return_yards, return_touchdown, fumble_lost)

# Compute net yards (kick distance - return yards)
punt_data <- punt_data %>%
  mutate(
    net_yards = kick_distance - coalesce(return_yards, 0),
    adjusted_net_yards = case_when(
      return_touchdown == 1 ~ -1100, # Punt return TD
      fumble_lost == 1 ~ 1100 + net_yards, # Kicking team recovers
      TRUE ~ net_yards
    )
  ) %>%
  select(yardline_100, adjusted_net_yards)

# Convert data to JSON format
punt_json <- toJSON(split(punt_data$adjusted_net_yards, punt_data$yardline_100), pretty = TRUE)

# Save to JSON file
write(punt_json, "punt_net_yards.json")

# Print confirmation message
print("JSON file 'punt_net_yards.json' successfully created with punt data.")
